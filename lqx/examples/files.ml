/*
 * Program built in 0.004000 Seconds.
 *
 * The path to this file is: /dev/null 
 * The open file is: File (Open @ /dev/null)
 * Type ID: 0 
 *
 *  Name:               Type:             Value:        
 * +-------------------+-----------------+--------------->
 *  fp                  object            File (Open @ /dev/null)
 *  path                string            /dev/null      
 *
 * The closed file is: File (Closed)
 * The path to this file is: (NULL) 
 *
 * Program ran in 0.018520 Seconds.
 */

/* Open a sample file */
file_open(fp, "/dev/null", read);
//path = file_get_path(fp);

/* Show some stuff */
println();
//println("The path to this file is:", file_get_path(fp));
//println("The open file is:", fp);
//println("Type ID:", get_type_id(fp));
println();
print_symbol_table();
println();

/* Close up the file */
file_close(fp);
//println("The closed file is:", fp);
//println("The path to this file is:", file_get_path(fp));
println();
