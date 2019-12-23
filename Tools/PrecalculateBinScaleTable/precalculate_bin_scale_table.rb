require "csv"
require "matrix"

# about Q NUMBERS
# http://www.hugi.scene.org/online/coding/hugi%2015%20-%20cmtadfix.htm
#
# about getting a frequency responce gaph from datasheet to CSV
# http://www.graphreader.com
#
# when the frequency response correction (extracted rom the specsheet) is applied we should have a flat response
# however, that flat response only applies for the bare microphone (without casing)
# therefore we also correct for the casing using white noise measurements, made with and without casing

# csv file with (freq, db) source for interpolation
MIC_RESPONSE_DATA = "freq_resp_SPH0645LM4H-B-RevB.csv"

# settings that should match the ones in SLMSettings.h
BINS = 1024
FS = 48000
Q_FRAC = 8
OUTPUT_QNUM = true
OUTPUT = :BIN_SCALE_TABLE # :BIN_SCALE_PLOT, :BIN_SCALE_TABLE

#
# Utility functions
#

# bin nr to frequency
def FFT_BIN num
  return (num*(FS.to_f/BINS.to_f))
end

# convert double to fixed point with fb bits for fractional part
def double2q x, fb
  (x * (1 << fb)).to_i
end

# return the a-weighting factor in DBs
def a_weight freq
  v = ((12200 ** 2) * (freq ** 4)) / (((freq ** 2) + (20.6 ** 2)) * ((freq ** 2) + (12200 ** 2)) * Math::sqrt((freq ** 2) + (107.7 ** 2)) * Math::sqrt((freq ** 2) + (737.9 ** 2)));
  return 20 * Math::log10(v) + 2;
end

# 6db ≈ 1.995
def db_to_amplitude_scale db
  return 10 ** (db / 20)
end

# 6db ≈ 3.98
def db_to_power_scale db
  return 10 ** (db / 10)
end

# check if each next x is bigger then the previous
def check_data_integrity

end

# use the table and linear interpolation to find values for the freqs of the bins
def interpolate x_in, xy_data
  x = nil
  y = nil
  xy_data.each_with_index do |point, index|
    x = point[0].to_f
    y = point[1].to_f
    if x >= x_in then
      break if index == 0 #just return lowest
      prev_point = xy_data[index-1]
      xp = prev_point[0].to_f
      yp = prev_point[1].to_f
      d = x - xp
      y = ((x_in - xp) / d) * y + ((x - x_in) / d) * yp
      break
    end
  end
  return y
end

# get gnuplot code
def get_plot_code filename, labels, args = nil
  plots = labels.map.with_index {|label, i| "'#{filename}' using 1:#{i+2} title '#{label}' #{args[i] || ""}"}.join(",")
  return "gnuplot -e \"set terminal png size 1920,1080;set logscale x 10;set yrange [-100:20];set term png;set output 'tables.png'; plot " + plots + "\""
end

def make_scale_table db_per_bin
  print "{"
  db_per_bin.each_with_index do |db, i|
    print "," if i > 0
    scale = db_to_power_scale(db)
    if OUTPUT_QNUM then
      print "0x#{double2q(scale, Q_FRAC).to_s(16)}"
    else
      print "#{scale}"
    end
  end
  print "}"
  puts ""
end


#
# bin vector generators
#


# get frequencies for the bins
def get_bin_freqs
  table = []
  (BINS/2).times do |i|
    table << FFT_BIN(i)
  end
  return Vector[*table]
end

# read spectrum csv file with freq in left col, db in right col
def read_spectrum path, col_sep, skip_rows = 0
  table = []
  data = CSV.read(path, {:col_sep => col_sep})
  data = data.drop(skip_rows)
  get_bin_freqs.each do |freq|
    table << interpolate(freq, data)
  end
  return Vector[*table]
end

# returns db's for each bin
def get_mic_response
  return read_spectrum MIC_RESPONSE_DATA, ","
end

# get the a weighting response
def get_a_weighting_response
  table = []
  get_bin_freqs.each do |freq|
    table << a_weight(freq)
  end
  return Vector[*table]
end

#
# calculate everything there is to calculate in vector having the size of BINS/2
#

# get all frequencies corresponding with the bins
bin_freqs        = get_bin_freqs
# get the microphone response as put in the specsheet by the manufacturer
mic_response     = get_mic_response
# get the a-weighting response as calculated
a_weighting      = get_a_weighting_response
# total correction
total_correction = a_weighting - mic_response

# various outputs
if OUTPUT == :BIN_SCALE_PLOT then
  cols = [bin_freqs.to_matrix, mic_response.to_matrix,  a_weighting.to_matrix, total_correction.to_matrix]
  matrix = Matrix.combine(*cols).to_a
  # output as csv
  puts matrix.map{|row| row.to_csv(col_sep:" ")}
  # plot code
  labels = ["mic response", "a-weighting", "total correction"]
  args = Array.new(labels.size)
  args.fill("w l")
  if STDOUT.tty? then
    STDERR.puts "\nuse like : ruby precalculate_bin_scale_table.rb > out.csv"
  else
    STDERR.puts "plot using:"
    STDERR.puts get_plot_code "out.csv", labels, args
  end
end

if OUTPUT == :BIN_SCALE_TABLE then
  make_scale_table total_correction.to_a
end
