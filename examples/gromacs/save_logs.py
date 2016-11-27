import os
import sys
import getopt



def main(argv):
   name = ''
   folder = 'Exp/'
   try:
      opts, args = getopt.getopt(argv,"hn:",["name="])
   except getopt.GetoptError:
      print 'save_logs.py -n <exp_name>'
      sys.exit(2)
   for opt, arg in opts:
      if opt == '-h':
         print 'save_logs.py -n <exp_name>'
         sys.exit()
      elif opt in ("-n", "--name"):
         name = arg

   if name == "":
       print 'save_logs.py -n <exp_name>'
       sys.exit(2)

   print 'Exp name is ', name
   folder = folder + name + "/"
   print 'Destination_folder is ', folder
   os.system("mkdir " + folder)
   os.system("cp *.csv " + folder)
   os.system("cp *.py " + folder)
   os.system("cp *.json " + folder)
   os.system("cp *.xml " + folder)
   os.system("cp log-* " + folder)
   os.system("cp stats_gmx_* " + folder)
   os.system("cp *.txt " + folder)
   os.system("cp FEPA_TEST_* " + folder)


if __name__ == "__main__":
   main(sys.argv[1:])
