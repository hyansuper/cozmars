#pragma once

int clamp_int(int val, int lo, int hi);
int abs_int(int val);

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))