/*
 * Program built in 0.011360 Seconds.
 *
 *   +-+ Outside the Compound Statement +-+
 *    Name:               Type:             Value:        
 *   +-------------------+-----------------+--------------->
 *  1   x                   double            4              
 *  1   y                   double            5              
 *  1   z                   double            6              
 *
 *    +-+ Within the Compound Statement +-+
 *     Name:               Type:             Value:        
 *    +-------------------+-----------------+--------------->
 *  1   a                   double            1.1            
 *  2   x                   double            99999          
 *  2   y                   double            5              
 *  2   z                   double            6              
 * 
 *    +-+ Outside the Compound Statement +-+
 *     Name:               Type:             Value:        
 *    +-------------------+-----------------+--------------->
 *  1   x                   double            99999          
 *  1   y                   double            5              
 *  1   z                   double            6              
 * 
 * Program ran in 0.012890 Seconds.
 */

/* Global Variables */
x = 4;
y = 5;
z = 6;

/* Print the Table */
println();
println("   +-+ Outside the Compound Statement +-+");
print_symbol_table();

{
  /* Define "a", assign "x" */
  a = 1.1;
  x = 99999;
  println("   +-+ Within the Compound Statement +-+");
  print_symbol_table();
}

/* Print the Table */
println("   +-+ Outside the Compound Statement +-+");
print_symbol_table();
