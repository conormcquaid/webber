#
# Usage: perl gif2c.pl input_gif_file [ > redirect_output_file ]
#
# Reads a monochrome gif file and generates a gcc compliant header for SSD1306 OLED
#
# Used to create collateral images for KMA
#




use GD;

$filename = $ARGV[0];


open (GIFFILE,$filename) || die "Please specify a GIF filename on the command line!\n";


$image = newFromGif GD::Image (\*GIFFILE) || die "Couldn't read GIF data\n";

close GIFFILE;

$numColors = $image->colorsTotal;

($width,$height) = $image->getBounds();

$filename =~ s/(\.gif)$//;


#first pass validate colors
$y_byte = ($height/8)-1;
if($y_byte < 0){ $y_byte = 0; }

$i2c_cnt = $width;
$lines = $y_byte+1;
print"const uint8_t $filename\[$lines\]\[$i2c_cnt\] = {\n";

foreach $y( 0 .. $y_byte){

print"   { ";

#for($x =$width; $x > 0; $x--){
foreach $x (0 .. ($width - 1)){

$val = 0 ;

foreach $bit(0 .. 7){

($r,$g,$b) = $image->rgb($image->getPixel($x,($y * 8)+$bit));

if( $r > 10 && $b > 10 && $g > 10){


$val |= (1 << $bit);
}
}
print "$val";
print"," unless $x == $width-1;
}
print"},\n";
}
print"};\n";







#open OUTFILE, ">new_$filename";
#binmode OUTFILE;
#print OUTFILE $image->gif;
#close OUTFILE;



