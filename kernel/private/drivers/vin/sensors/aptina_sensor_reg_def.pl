#!/usr/bin/perl -w
#
# filter1.pl
#
# Extract the register define from the *.sdat, which can be found in the setup directory of aptina DevWare
#
# NOTE: a) Strings containing "/" must have them escaped with a backslash,
#       just as in a Perl substitution string, or the substitution command
#       will probably fail.
#		b) You should set the *.sdat file to unix format before doing the
#		filter job(use cmd:"sed 's/^V^M//g' foo > foo.new"  or use the vim "set ff=unix" and "wq"
#
#
# History:
#      2009/05/19 - [Qiao Wang] Create
#
# Copyright (C) 2004-2009, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

$SdatFile = $ARGV[0];
$INFILE = "";
$OUTFILE= "";

#time
$sec = "";
$min = "";
$hour = "";
$day = "31";
$mon = "05";
$year = "2009";
$weekday = "";
$yeardate = "";
$savinglightday = "";
$now = "";
print "
	Extract the register setting define, and
produe C register/bits mask defines for sensor aptina
driver development.


						--Qiao (Ambarella SH)
						--12/01/2009(update)

";
&main();
print "\n Job is done!\n";

sub main
{
	&getTime();

	open(INFILE, "$SdatFile") or die "Can not open $SdatFile $!\n";

	# replace *.sdat to *_pri.h
	$_ = "$SdatFile";
	if(!(s#\.sdat#\_pri.h#))
		{print " Replace error! \n Make sure your input file is *.sdat\n"};

	my $hfile = "$_"; #new file name
	#$hfile =~ s#^.*\/##s;#remove the path TODO
	$hfile =~ s#(.*)-.*_#\L$1_#gi;#shot the -rev, and lowcase it
	open(OUTFILE, ">$hfile") or die "Can not open $hfile$!\n";

	my $prefix = $hfile;
	$prefix =~ s#(.*_)pri.h#$1#gi;
	my $Uprefix = $hfile;
	$Uprefix =~ s#(.*_)pri.h#\U$1#gi;
	#produce the head
	printf OUTFILE  "\
/*
 * Filename : $hfile
 *
 * History:
 *    $year/$mon/$day - [Qiao Wang] Create
 *
 * Copyright (C) 2004-$year, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is created by perl.
 */\n";

	my @outLines;  # Data we are going to output
	my @inLines; # Data we are reading line by line
	@inLines = <INFILE>;
	$count = 1;
	$number = @inLines;

	for(; $count < $number; $count++){##remove the sdat head comment
		$line = $inLines[$count];
		if($line =~ /^\/\//){
			next;
		}else{
			last;
		}
	}
	$count++;

	for(; $count < $number; $count++){#comment out the information
		$line = $inLines[$count];
		if(!($line =~ /^\[REGISTERS\]/)){
			$line = "//".$line;
			push(@outLines, $line);
			next;
		}else{
			last;
		}
	}
	$count++;
	print"count = $count\n";
	print"number= $number\n";
	for(; $count < $number; $count++){ #split start
		$line = $inLines[$count];
		if(!($line =~ /^\[END\]/)){
			if(!($line =~ /=.*{/)){#bit define
				$line =~ s/^ *{(\w+),( *)(0x\w\w\w\w),(.*)/\t#define $Uprefix$1_MASK $2 $3\t\/\*$4 \*\//i; #TODO revmoe hard code MT9M033
			}else{#register define
				$line =~ s/^(\w+)(.*{)(0x\w\w\w\w),(.*)/#define $Uprefix$1$2 $3\t\/\*$4 \*\//i;
				$line =~ s/=/ /i;
			}
			$line =~ s/{/ /i;
			push(@outLines, $line);
			next;
		}else{
			last;
		}
	}

	close INFILE;
	print(OUTFILE @outLines);
	close OUTFILE;
	undef( @outLines );
	undef( @inLines );
}
# Get the time
sub getTime()
{
	($sec,$min,$hour,$day,$mon,$year,$weekday,$yeardate,$savinglightday) = (localtime(time));
	$sec   =   ($sec   <   10)?   "0$sec":$sec;
	$min   =   ($min   <   10)?   "0$min":$min;
	$hour  =   ($hour  <   10)?   "0$hour":$hour;
	$day   =   ($day   <   10)?   "0$day":$day;
	$mon   =   ($mon   <    9)?   "0".($mon+1):($mon+1);
	$year +=   1900;
	$now   =   "$year-$mon-$day   $hour:$min:$sec";
}

# end.
