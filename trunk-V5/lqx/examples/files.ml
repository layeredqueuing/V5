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
fp = file_open("/dev/null", "rb");
path = file_get_path(fp);

/* Show some stuff */
print();
print("The path to this file is:", file_get_path(fp));
print("The open file is:", fp);
print("Type ID:", get_type_id(fp));
print();
print_symbol_table();
print();

/* Close up the file */
file_close(fp);
print("The closed file is:", fp);
print("The path to this file is:", file_get_path(fp));
print();
