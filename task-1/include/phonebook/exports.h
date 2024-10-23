#pragma once

#include "phonebook/phonebook.h"

long pb_get_user(const char* surname, unsigned int len, struct pb_user_data* output_data);

long pb_add_user(struct pb_user_data* input_data);

// Удаляет всех пользователей с такой фамилией.
long pb_del_user(const char* surname, unsigned int len);