#! /usr/bin/perl -w
# 
# create a tar ball snapshot from cvs repository.
# oh, more bash than perl here....
#
# 2005-12-15, jw
#

use Data::Dumper;

my $srcdir = 'src';
my $version_file = "$srcdir/patchlevel.h";
my %version;
open IN, "<", $version_file or die "unable to read $version_file: $!";
while (defined(my $line = <IN>))
  {
    $version{$1} = (defined $3) ? $3 : $4 if $line =~ m{^#\s*define\s+(\w+)\s+("([^"]*)"|(\S+))};
  }
close IN;

my $version = "$version{REV}.$version{VERS}.$version{PATCHLEVEL}";
my $tmpdir = "/tmp/mktar-$^T";
mkdir $tmpdir or die "cannot mkdir $tmpdir: $!";
system "cp -a $srcdir $tmpdir" and die "'cp -a $srcdir $tmpdir' failed: $!";
rename "$tmpdir/$srcdir", "$tmpdir/screen-$version" or die "rename to screen-$version failed: $!";

system "tar zcvf - -C $tmpdir > screen-$version.tar.gz screen-$version"
system "rm -rf $tmpdir";
