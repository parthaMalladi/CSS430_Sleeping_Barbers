#include "Shop.h"

void Shop::init() 
{
   pthread_mutex_init(&mutex_, NULL);

   for (int i = 0; i < max_barbers_; i++) {
      Barber* b = new Barber();
      b->myId = i;
      barbers[i] = b;
   }
}

Shop::~Shop() {
   for (auto it = barbers.begin(); it != barbers.end(); ++it) {
      delete it->second;
   }

   for (auto it = customers.begin(); it != customers.end(); ++it) {
      delete it->second;
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

   if (waiting_chairs_.size() == max_waiting_cust_) {
      printCustomer(id, "leaves the shop because of no available waiting chairs.");
      ++custDrops;
      pthread_mutex_unlock(&mutex_);
      return barber;
   }

   Customer* c = new Customer();
   c->myId = id;
   customers[id] = c;

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

   customers[id]->isGettingServiced = true;
   pthread_cond_signal(&barbers[barber]->barberSignal);
   pthread_mutex_unlock(&mutex_);

   return barber;
}

void Shop::leaveShop(int id, int barber_id) 
{
   pthread_mutex_lock(&mutex_);
   printCustomer(id, "wait for the hair-cut to be done");

   while (customers[id]->myBarber != -1) {
      pthread_cond_wait(&barbers[barber_id]->barberSignal, &mutex_);
   }

   customers[id]->isGettingServiced = false;
   customers[id]->hasPaid = true;
   printCustomer(id, "says good-bye to the barber.");

   pthread_mutex_unlock(&mutex_);
}

void Shop::helloCustomer(int barber_id) 
{
   pthread_mutex_lock(&mutex_);
}

void Shop::byeCustomer(int barber_id) 
{

}
