/*
 * Program built in 0.003380 Seconds.
 * 2 
 * 3 
 * 5 
 * 8 
 * 13 
 * 21 
 * 34 
 * 55 
 * 89 
 * 144 
 * 233 
 * 377 
 * 610 
 * 987 
 * 1597 
 * 2584 
 * 4181 
 * 6765 
 * 10946 
 * 17711 
 * 28657 
 * 46368 
 * 75025 
 * 121393 
 * Program ran in 0.016920 Seconds.
 */

fib_before_last = 1;
fib_last = 1;
fib = 0;

/* Compute fibonnaci for a while */
while ( fib < 100000 ) {
  fib = fib_before_last + fib_last;
  fib_before_last = fib_last;
  fib_last = fib;
  println(fib);
}
