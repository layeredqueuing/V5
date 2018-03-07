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
 
file_open(fp, "/dev/null", read);
/* Create a new vector */
x = vector_new();
vector_push_back(x, 111);
vector_push_back(x, true);
vector_push_back(x, fp);
vector_put(x, 0, "Replaced");
vector_push_back(x, 111);

/* Do some fun printing */
println();
println("V[0] =", vector_get(x, 0));
println("V[1] =", vector_get(x, 1));
println("V[2] =", vector_get(x, 2));
println("V[3] =", vector_get(x, 3));
println("Size =", vector_get_size(x));

/* Print the symbol table */
println();
print_symbol_table();
