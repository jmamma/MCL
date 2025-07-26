#pragma once

#include "Arduino.h"

/** Store the name of a monomachine machine. **/
typedef struct mnm_machine_name_s {
  char name[11];
  uint8_t id; 
} mnm_machine_name_t;

/** Store the name of a machinedrum machine. **/
typedef struct md_machine_name_s {
  char name[7];
  uint8_t id; 
} md_machine_name_t;

/** Store the name of a parameter for a machine model. **/
typedef struct model_param_name_s {
  char name[4];
  uint8_t id; 
} model_param_name_t;

/** Data structure holding the parameter names for a machine model. **/
typedef struct model_to_param_names_s {
  uint8_t model;
  uint16_t offset; // offset of the first param in the lookup table
} model_to_param_names_t;

typedef struct short_machine_name_s {
  char name1[3];
  char name2[3];
  uint8_t id; 
} short_machine_name_t;
