/*
 * Program built in 0.006890 Seconds.
 *
 * 0 => 1 
 * 1 => true 
 * 2 => string 
 * 3 => Array
 *
 *   Name:               Type:             Value:        
 *  +-------------------+-----------------+--------------->
 *  a                   object            Array          
 *  array               object            Array          
 *  key                 double            3              
 *  value               object            Array          
 *  x                   double            1              
 *  y                   boolean           true           
 *  z                   string            string         
 * 
 * Program ran in 0.007770 Seconds.
 *
 */

/* Write some variables */
x = 1;
y = true;
z = "string";
a = array_create();

/* Put them all into an array */
array = array_create();
array[0] = x;
array[x] = y;
array[2] = z;
array[3] = a;
println();

/* Print out the contents of the array */
foreach (key,value in array) { 
  println(key, " => ", array[key]);
}

/* Write the table */
println();
print_symbol_table();
