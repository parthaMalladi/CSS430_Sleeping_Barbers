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

Shop::Barber* Shop::getBarber(int barber_id)
{
    for (int i = 0; i < max_barbers_; i++)
    {
        if (barbers[i]->myId == barber_id)
        {
            return barbers[i];
        }
    }

    return NULL;
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
   customers[id].myId = id;

   if (available_barbers_.empty()) {
      waiting_chairs_.push(id);
      printCustomer(id, "takes a waiting chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

      while (customers[id].myBarber == -1) {
         pthread_cond_wait(&customers[id].customerSignal, &mutex_);
      }
      barber = customers[id].myBarber;
   } else {
      barber = available_barbers_.front();
      available_barbers_.pop();
      customers[id].myBarber = barber;
      getBarber(barber)->myCustomer = id;
   }

   printCustomer(id, "moves to the service chair. # waiting seats available = " + int2string(max_waiting_cust_ - waiting_chairs_.size()));

   customers[id].state = 2;
   pthread_cond_signal(&getBarber(barber)->barberSignal);
   pthread_mutex_unlock(&mutex_);

   return barber;
}

void Shop::leaveShop(int id, int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   printCustomer(id, "wait for the hair-cut to be done");

   while (customers[id].myBarber != -1) {
      pthread_cond_wait(&customers[id].customerSignal, &mutex_);
   }

   printCustomer(id, "says good-bye to the barber.");
   customers[id].state = 3;
   pthread_cond_signal(&getBarber(barber_id)->barberSignal);

   pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);

   if (getBarber(barber_id)->myCustomer == -1) {
      printBarber(barber_id, "sleeps because of no customers.");
      available_barbers_.push(barber_id);

      while (getBarber(barber_id)->myCustomer == -1) {
         pthread_cond_wait(&getBarber(barber_id)->barberSignal, &mutex_);
      }
   }

   while (customers[getBarber(barber_id)->myCustomer].state != 2) {
      pthread_cond_wait(&getBarber(barber_id)->barberSignal, &mutex_);
   }

   printBarber(barber_id, "starts a hair-cut service for " + int2string(getBarber(barber_id)->myCustomer));
   pthread_mutex_unlock(&mutex_);
}

void Shop::byeCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   printBarber(barber_id, "says he's done with a hair-cut service for " + int2string(getBarber(barber_id)->myCustomer));

   customers[getBarber(barber_id)->myCustomer].myBarber = -1;

   pthread_cond_signal(&customers[getBarber(barber_id)->myCustomer].customerSignal);

   while (customers[getBarber(barber_id)->myCustomer].state != 3) {
      pthread_cond_wait(&getBarber(barber_id)->barberSignal, &mutex_);
   }

   getBarber(barber_id)->myCustomer = -1;
   printBarber(barber_id, "calls in another customer");

   if (!waiting_chairs_.empty()) {
      int client = waiting_chairs_.front();
      waiting_chairs_.pop();
      
      getBarber(barber_id)->myCustomer = client;
      customers[client].myBarber = barber_id;
      pthread_cond_signal(&customers[client].customerSignal);
   }

   pthread_mutex_unlock( &mutex_ );
}
