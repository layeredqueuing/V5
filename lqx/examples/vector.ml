
/*
 * Program built in 0.007090 Seconds.
 *
 * V[0] = Replaced 
 * V[1] = true 
 * V[2] = File (Open @ /dev/null)
 * V[3] = 111 
 * Size = 4 
 *
 *  Name:               Type:             Value:        
 * +-------------------+-----------------+--------------->
 *  x                   object            Vector         
 *
 * Program ran in 0.005920 Seconds.
 *
 */
 
/* Create a new vector */
x = vector_new();
vector_push_back(x, 111);
vector_push_back(x, true);
vector_push_back(x, file_open("/dev/null", "rb"));
vector_put(x, 0, "Replaced");
vector_push_back(x, 111);

/* Do some fun printing */
print();
print("V[0] =", vector_get(x, 0));
print("V[1] =", vector_get(x, 1));
print("V[2] =", vector_get(x, 2));
print("V[3] =", vector_get(x, 3));
print("Size =", vector_get_size(x));

/* Print the symbol table */
print();
print_symbol_table();
