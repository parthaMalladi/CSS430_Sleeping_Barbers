#include "Shop.h"

void Shop::init() 
{
   pthread_mutex_init(&mutex_, NULL);

   for (int i = 0; i < max_barbers_; i++) {
      Barber* b = new Barber();
      b->myId = i;
      barbers.push_back(b);
   }
}

Shop::~Shop() {
   for (int i = 0; i < max_barbers_; i++) {
      delete barbers[i];
   }
}

string Shop::int2string(int i) 
{
   stringstream out;
   out << i;
   return out.str( );
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

   if (waiting_chairs_.size() == max_waiting_cust_ && available_barbers_.empty()) {
      printCustomer(id, "leaves the shop because of no available waiting chairs.");
      ++custDrops;
      pthread_mutex_unlock(&mutex_);
      return barber;
   }

   customers[id] = Customer();

   if (available_barbers_.empty()) {
      waiting_chairs_.push(id);
      printCustomer(id, "takes a waiting chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

      while (customers[id]->myBarber == -1) {
         pthread_cond_wait(&customers[id]->customerSignal, &mutex_);
      }
      barber = customers[id]->myBarber;
   } else {
      barber = available_barbers_.front();
      available_barbers_.pop();
      customers[id]->myBarber = barber;
      barbers[barber]->myCustomer = id;
   }

   printCustomer(id, "moves to the service chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

   customers[id]->state = 2;
   pthread_cond_signal(&barbers[barber]->barberSignal);
   pthread_mutex_unlock(&mutex_);

   return barber;
}

void Shop::leaveShop(int id, int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   printCustomer(id, "wait for the hair-cut to be done");

   while (customers[id]->myBarber != -1) {
      pthread_cond_wait(&customers[id]->customerSignal, &mutex_);
   }

   customers[id]->state = 3;

   printCustomer(id, "says good-bye to the barber.");
   pthread_cond_signal(&barbers[barber_id]->barberSignal);

   pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);

   if (barbers[barber_id]->myCustomer == -1) {
      printBarber(barber_id, "sleeps because of no customers.");
      available_barbers_.push(barber_id);

      while (barbers[barber_id]->myCustomer == -1) {
         pthread_cond_wait(&barbers[barber_id]->barberSignal, &mutex_);
      }
   }

   while (customers[barbers[barber_id]->myCustomer]->state != 2) {
      pthread_cond_wait(&barbers[barber_id]->barberSignal, &mutex_);
   }

   printBarber(barber_id, "starts a hair-cut service for " + int2string(barbers[barber_id]->myCustomer));
   pthread_mutex_unlock( &mutex_ );
}

void Shop::byeCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   int currCustomer = barbers[barber_id]->myCustomer;
   printBarber(barber_id, "says he's done with a hair-cut service for " + int2string(currCustomer));

   customers[currCustomer]->myBarber = -1;

   pthread_cond_signal(&customers[currCustomer]->customerSignal);

   while (customers[currCustomer]->state != 3) {
      pthread_cond_wait(&barbers[barber_id]->barberSignal, &mutex_);
   }

   barbers[barber_id]->myCustomer = -1;
   printBarber(barber_id, "calls in another customer");

   if (!waiting_chairs_.empty()) {
      int client = waiting_chairs_.front();
      waiting_chairs_.pop();
      
      Customer* c = new Customer();
      c->myId = client;
      customers[client] = c;
      barbers[barber_id]->myCustomer = client;
      customers[client]->myBarber = barber_id;

      pthread_cond_signal(&customers[client]->customerSignal);
   }

   pthread_mutex_unlock( &mutex_ );
}
