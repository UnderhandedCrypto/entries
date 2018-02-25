Real-life CSV data files downloaded from:

	https://vincentarelbundock.github.io/Rdatasets/

Some good lists, included here:

	 lines filename          description
	 ----- --------          -----------
	147850 hr1420.csv        An example data for Manhattan plot with annotation
	 21700 baseball.csv      Yearly batting records for all major league baseball players
	 21639 Vocab.csv         Vocabulary and Education
	  8089 NOxEmissions.csv  NOx Air Pollution Data
	  5749 Star.csv          Effects on Learning of Small Class Sizes
	  4945 first-names.txt   English first names
	  3127 mid.csv           Militarized Interstate Disputes
	  3001 Wage.csv          Mid-Atlantic Wage Data
	  2801 bfi.csv           25 Personality items representing 5 factors
	  1526 iqitems.csv       16 multiple choice IQ items
	  1526 ability.csv       16 ability items scored as correct or incorrect
	  1072 datasets.csv      Index of datasets in Rdatasets

They are good files for the reason that they don't have a natural, logical
sequential "ID" field that would give itself to sorting, and has enough lines
to contain at least a basic email-size message.

The only modification I've made to these CSV files is that I've taken off any
CSV header -- while it would be trivially easy to amend the encoders and
decoders to deal with (ignore) a header line, I didn't want to unecessarily add
any boilerplate code to keep the demonstration simple, so this is left as an
exercise for the reader.

Another interesting source for cover data is:

	https://mockaroo.com/
