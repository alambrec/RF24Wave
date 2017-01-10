#ifndef __MAIN_H
#define __MAIN_H

#define CE_PIN 7
#define CSN_PIN 8

#define NODE_ID 1

struct payload_t {
  unsigned long ms;
  unsigned long counter;
};

#endif
