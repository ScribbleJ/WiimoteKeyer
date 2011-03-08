# Quick perl script meant to give you an idea how often you use which letters.
#!/usr/bin/perl
use strict;

my %lc=();

while (my $str = <STDIN>)
{
  my @ls = split //, $str;
  foreach my $l (@ls)
  {
    if(ord($l) >= 32 and ord($l) <= 127)
    {
      $lc{lc($l)}++;
    }
  }
}

print map(sprintf("%10d %s\n", $lc{$_}, $_), sort {$lc{$b} <=> $lc{$a}}  keys(%lc));
