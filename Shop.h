#ifndef SHOP_H_
#define SHOP_H_
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <map>
using namespace std;

#define kDefaultNumChairs 3
#define kDefaultNumBarbers 1

class Shop
{
public:
   Shop(int num_barbers, int num_chairs) : 
      max_waiting_cust_(num_chairs),
      max_barbers_(num_barbers),
      custDrops(0)
   { 
      init(); 
   };

   Shop() : 
      max_waiting_cust_(kDefaultNumChairs),
      max_barbers_(kDefaultNumBarbers),
      custDrops(0)
   { 
      init(); 
   };

   ~Shop();

   int visitShop(int id);
   void leaveShop(int id, int barber_id);
   void helloCustomer(int barber_id);
   void byeCustomer(int barber_id);
   int get_cust_drops() const;

 private:
   pthread_mutex_t mutex_;
   const int max_waiting_cust_;
   const int max_barbers_;
   queue<int> waiting_chairs_;
   queue<int> available_barbers_;
   int custDrops;

   struct Customer {
      int myId;
      int myBarber;
      bool gettingHaircut;
      bool paid;
      pthread_cond_t customerSignal;

      Customer() {
         myId = -1;
         myBarber = -1;
         gettingHaircut = false;
         paid = false;
         pthread_cond_init(&customerSignal, NULL);
      }
   };

   struct Barber {
      int myId;
      int myCustomer;
      pthread_cond_t barberSignal;

      Barber() {
         myId = -1;
         myCustomer = -1;
         pthread_cond_init(&barberSignal, NULL);
      }
   };

   vector<Barber*> barbers;
   map<int, Customer> customers;
  
   void init();
   string int2string(int i);
   void printCustomer(int person, string message);
   void printBarber(int person, string message);
};
#endif