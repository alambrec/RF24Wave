#ifndef __NODE_H
#define __NODE_H

#define CE_PIN 9
#define CSN_PIN 10

#define NODE_ID 2

struct payload_t {
  unsigned long ms;
  unsigned long counter;
};

#endif
