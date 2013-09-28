BEGIN {
  maxlqns = 0;
  minlqns = 0;
  lqns90  = 0;
  n       = 0;
}
{
  count += 1;
  lqns = $1;
  if ( lqns < 0 ) {
    lqns = -$1;
  }
  lqns_sum += lqns; 
  lqns_sqr += lqns * lqns;
  n += 1;
  if ( n == 49 ) {
    lqns90 = lqns;
  }
  if ( lqns > maxlqns ) {
    maxlqns = lqns;
  }
  if ( lqns < minlqns ) {
    minlqns = lqns;
  }
} 
END { 
  print "Mean:   ", lqns_sum / n;
  print "Stddev: ", (lqns_sqr - lqns_sum * lqns_sum / n) / (n * (n-1));
  print "90th %: ", lqns90;
  print "Max:    ", maxlqns;
  print "Min:    ", minlqns;
}
