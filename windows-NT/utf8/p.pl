#!/usr/bin/perl

$last1="";
$last2="";
open(HDR,$ARGV[0]) || die "Couldn't open header";
print "#include \"$ARGV[0]\"\n\n";
while(<HDR>)
{
  chop;
  if(/U *\(/)
  {
    $exit="";
    print "$last2 $last1 ";
    $string = "";
    do
    {
	s/\s+/ /g;
	if(/\);/)
        {
	  $exit=1;
        }
	s/;//g;
	print $_,"\n";
	s/U\s*\(/W(/;
	s/IN\s+//;
	s/OUT\s+//;
	s/LPSTR\s+([^,\)]*)([,\)])/UTF8_MAP(\1)\2/g;
	s/LPCSTR\s+([^,\)]*)([,\)])/UTF8_C_MAP(\1)\2/g;
	s/LPSTR\s+([^\s,\)]*)$/UTF8_MAP(\1)/g;
	s/LPCSTR\s+([^\s,\)]*)$/UTF8_C_MAP(\1)/g;
	s/(.*)\s+([^\s,\)]*),/\2,/g;
	s/(.*)\s+([^\s,\)]*)$/\2/g;
	s/\*//g;
	s/,\s*\)/)/g;
        $string .=$_;
	unless($exit)
        {
	  $_=<HDR>;
	  chop;
        }
    } until $exit;
    print "{\nreturn $string;\n}\n\n";
  }
  $last2=$last1;
  $last1=$_;
}
