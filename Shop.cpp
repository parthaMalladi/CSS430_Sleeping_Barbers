#include "Shop.h"

// intialize barbers where the index in the vector corresponds to the barber's id
void Shop::init() 
{
    pthread_mutex_init(&mutex_, nullptr);
    
    int barberId = 0;
    while (barberId < max_barbers_) {
      Barber* newBarber = new Barber();
      newBarber->myId = barberId;
      barbers.push_back(newBarber);
      ++barberId;
    }
}

// destructor for deleting the barbers
Shop::~Shop() 
{
    for (Barber* b : barbers) {
      delete b;
    }
}

string Shop::int2string(int i) 
{
    stringstream stream;
    stream << i;
    return stream.str();
}

void Shop::printBarber(int person, string message)
{
    cout << "barber[" << person << "]: " << message << endl;
}

void Shop::printCustomer(int person, string message)
{
    cout << "customer[" << person << "]: " << message << endl;
}

int Shop::get_cust_drops() const 
{
    return custDrops;
}

int Shop::visitShop(int id) 
{
    pthread_mutex_lock(&mutex_);
    int barber = -1;

    // leave if no waiting chairs or barbers
    if (waiting_chairs_.size() >= max_waiting_cust_ && available_barbers_.empty()) {
        ++custDrops;
        printCustomer(id, "leaves the shop because of no available waiting chairs.");
        pthread_mutex_unlock(&mutex_);
        return barber;
    }
    
    // make a new customer and assign it an id
    Customer newCustomer;
    newCustomer.myId = id;
    customers[id] = newCustomer;
   
    // if there are waiting chairs but no barbers, take a seat and wait till a barber is available
    if (available_barbers_.empty()) {
        waiting_chairs_.push(id);
        int remainingSeats = max_waiting_cust_ - waiting_chairs_.size();
        printCustomer(id, "takes a waiting chair. # waiting seats available = " + int2string(remainingSeats));

        while (customers[id].myBarber == -1) {
            pthread_cond_wait(&customers[id].customerSignal, &mutex_);
        }
        barber = customers[id].myBarber;
    } else {   // if there are barbers available, take a barber from the sleeping queue and assign to the customer
        barber = available_barbers_.front();
        available_barbers_.pop();
        customers[id].myBarber = barber;
        barbers[barber]->myCustomer = id;
    }
    
    int remainingSeats = max_waiting_cust_ - waiting_chairs_.size();
    printCustomer(id, "moves to the service chair. # waiting seats available = " + int2string(remainingSeats));
    
    // set the gettingHaircut state to true
    customers[id].gettingHaircut = true;

    // signal barber that they got assigned a customer in case they are asleep
    pthread_cond_signal(&barbers[barber]->barberSignal);
    pthread_mutex_unlock(&mutex_);
    return barber;
}

void Shop::leaveShop(int id, int barber_id) 
{
    pthread_mutex_lock(&mutex_);
    printCustomer(id, "wait for the hair-cut to be done");
    
    // wait for barber to finish service job before leaving
    while (customers[id].myBarber != -1) {
        pthread_cond_wait(&customers[id].customerSignal, &mutex_);
    }
    
    // pay the barber and signal that the barber is now free to attent to a new customer
    printCustomer(id, "says good-bye to the barber.");
    customers[id].paid = true;
    customers[id].gettingHaircut = false;
    pthread_cond_signal(&barbers[barber_id]->barberSignal);
    pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) 
{
    pthread_mutex_lock(&mutex_);
    
    // if the barber has no customer, go to sleep by going to the available queue
    Barber* b = barbers[barber_id];
    if (b->myCustomer == -1) {
        printBarber(barber_id, "sleeps because of no customers.");
        available_barbers_.push(barber_id);
        
        // sleep while woken up by a customer/get a customer assigned to you
        while (b->myCustomer == -1) {
            pthread_cond_wait(&b->barberSignal, &mutex_);
        }
    }
    
    // wait while service gets initiated by the customer
    int customerId = b->myCustomer;
    while (customers[customerId].gettingHaircut == false) {
        pthread_cond_wait(&b->barberSignal, &mutex_);
    }

    printBarber(barber_id, "starts a hair-cut service for " + int2string(customerId));
    pthread_mutex_unlock(&mutex_);
}

void Shop::byeCustomer(int barber_id) 
{
    pthread_mutex_lock(&mutex_);

    Barber* b = barbers[barber_id];
    int customerId = b->myCustomer;

    printBarber(barber_id, "says he's done with a hair-cut service for " + int2string(customerId));

    // complete haircut and remove assigned barber from customer and signal for payment
    customers[customerId].myBarber = -1;
    pthread_cond_signal(&customers[customerId].customerSignal);
    
    // wait while barber hasn't gotten paid
    while (customers[customerId].paid == false) {
        pthread_cond_wait(&b->barberSignal, &mutex_);
    }

    // once paid, the barber has no customer assigned
    b->myCustomer = -1;
    printBarber(barber_id, "calls in another customer");
    
    // if there are customers waiting take one of the customers from the waiting queue and signal that they can take this barber
    if (!waiting_chairs_.empty()) {
        int nextCustomer = waiting_chairs_.front();
        waiting_chairs_.pop();
        b->myCustomer = nextCustomer;
        customers[nextCustomer].myBarber = barber_id;
        pthread_cond_signal(&customers[nextCustomer].customerSignal);
    }

    pthread_mutex_unlock(&mutex_);
}