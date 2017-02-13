import unittest
import subprocess
import os
import glob

class Test_Client_rirods(unittest.TestCase):

    def setUp(self):
        num_files = 2
        subprocess.call(["imkdir", "-p", "/tempZone/UNIT_TESTING/EmptyCollection"])
        subprocess.call(["imkdir", "-p", "/tempZone/UNIT_TESTING/Data/Proteomics"])
        subprocess.call(["imkdir", "-p", "/tempZone/UNIT_TESTING/Data/Transcriptomics"])
        subprocess.call(["imkdir", "-p", "/tempZone/UNIT_TESTING/Analysis/testauthor/Proteomics"])
        subprocess.call(["imkdir", "-p", "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/indices"])
        subprocess.call(["imeta", "add", "-C", "/tempZone/UNIT_TESTING/EmptyCollection", "A", "V", "U"])
        # Create file to use as dummy data
        with open('/tmp/dummydata', 'w') as datafile:
            datafile.write("This is dummy data\nThe quick brown fox jumps over the lazy dog\n")
        for i in range(1,num_files+1):
            subprocess.call(["iput", "/tmp/dummydata", "/tempZone/UNIT_TESTING/Data/Proteomics/Sample"+`i`+".raw", "--metadata", "Project;UNIT_TESTING;;File type;Raw data file;;File format;Thermo Xcalibur raw file;;Sample;"+str(i)+";;"])
        for i in range(1,num_files+1):
            subprocess.call(["iput", "/tmp/dummydata", "/tempZone/UNIT_TESTING/Data/Transcriptomics/Sample"+`i`+".fastq", "--metadata", "Project;UNIT_TESTING;;File type;Raw data file;;File format;FASTQ;;Sample;"+str(i)+";;"])
        for i in range(1,num_files+1):
            subprocess.call(["iput", "/tmp/dummydata", "/tempZone/UNIT_TESTING/Analysis/testauthor/Proteomics/Sample"+`i`+".dat", "--metadata", "Project;UNIT_TESTING;;File type;Raw data file;;File format;Mascot report;;Sample;"+str(i)+";;"])
        for i in range(1,num_files+1):
            subprocess.call(["iput", "/tmp/dummydata", "/tempZone/UNIT_TESTING/Analysis/testauthor/Proteomics/Sample"+`i`+".sf3", "--metadata", "Project;UNIT_TESTING;;File type;Raw data file;;File format;Scaffold binary data format;;Sample;"+str(i)+";;"])
        for i in range(1,num_files+1):
            subprocess.call(["iput", "/tmp/dummydata", "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/Sample"+`i`+".bam", "--metadata", "Project;UNIT_TESTING;;File type;Raw data file;;File format;BAM;;Sample;"+str(i)+";;"])
        subprocess.call(["iput", "/tmp/dummydata", "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts", "--metadata", "Project;UNIT_TESTING;;File type;Raw data file;;File format;Comma-separated value format;;"])
        super(Test_Client_rirods, self).setUp()

    def tearDown(self):
        super(Test_Client_rirods, self).tearDown()
        subprocess.call(["irm", "-rf", "/tempZone/UNIT_TESTING"])
        subprocess.call(["irmtrash"])

def create_test(r_test_script):
    def test(self):
        with open(r_test_script) as rscript:
            proc = subprocess.Popen(["R",  "--no-save", "--slave", "-q"], stdin=rscript, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out, err) = proc.communicate()
        self.assertEqual(0, proc.returncode, '{0} issues\nout[{1}]\nstderr[{2}]'.format(proc.returncode, out, err))
    return test

for r_test_script in glob.glob(os.path.dirname(os.path.realpath(__file__)) + "/test*.R"):
    test_method = create_test(r_test_script)
    test_method.__name__ = (r_test_script.split('/')[-1])[:-2]
    setattr (Test_Client_rirods, test_method.__name__, test_method)
