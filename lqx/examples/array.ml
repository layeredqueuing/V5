
file = file_open("/dev/null", "rb");
x = array_create();
x[true] = 1;
array_set(x, file, 0);
file_close(file);
array_set(x, file, 0);
print("M: Array[true] = ", array_get(x, true));
print("I: Array[true] = ", x[true]);

print();
print_symbol_table();
