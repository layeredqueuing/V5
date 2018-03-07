
file_open(file, "/dev/null", read );
x = array_create();
x[true] = 1;
array_set(x, file, 0);
file_close(file);
println("M: Array[true] = ", array_get(x, true));
println("I: Array[true] = ", x[true]);

println();
print_symbol_table();
