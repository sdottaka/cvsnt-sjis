#!/usr/bin/env python
import os
import sys
import fileinput
import shutil
import filecmp
import getopt

def cvs(command):
  #cmd = 'valgrind -q --logfile-fd=9 cvs -D'+base_dir+' -d'+current_cvsroot+' '+command+' >'+outfile+' 2>'+errfile+' 9>/dev/stderr'
  cmd = 'cvs -D'+base_dir+' -d'+current_cvsroot+' '+command+' >'+outfile+' 2>'+errfile
  if(verbose): print cmd
  result = os.system(cmd)
#  if(verbose):
#    cat(outfile)
#    cat(errfile)
  return result

def cat(file):
  for line in fileinput.input(file):
    if line and line[-1] == '\n':
      line = line[:-1]
    print line

def fail(command, result):
  print 'Test \''+current_test+'\' failed ('+command+') (result='+str(result)+')'
  cat (errfile)
  raise SystemExit

def cvs_pass(command):
  result = cvs(command)
  if(result != 0): fail(command,result)

def cvs_fail(command):
  result = cvs(command)
  if(result == 0): fail(command,result)

def start_test(name):
  global current_test
  current_test = name
  print current_test

def dir_exists(dirname):
  if(not os.path.isdir(dirname)):
    print 'Directory '+dirname+' Which should exist, doesn\'t.  Terminating.'
    raise SystemExit

def file_exists(filename):
  if(not os.path.isfile(filename)):
    print 'File '+filename+' Which should exist, doesn\'t.  Terminating.'
    raise SystemExit

def file_not_exists(filename):
  if(os.path.isfile(filename)):
    print 'File '+filename+' Which shouldn\'t exist, does.  Terminating.'
    raise SystemExit

def file_copy(srcfile,destfile):
  if(verbose): print "Copy "+srcfile+" -> "+destfile
  shutil.copyfile(srcfile,destfile) 

def file_compare(file1,file2):
  if(verbose): print "Compare "+file1+" -> "+file2
  if(filecmp.cmp(file1,file2) == 0):
    print 'File '+file1+' Should be identical to File '+file2+'. Terminating.'
    raise SystemExit

def file_delete(filename):
  if(verbose): print "Delete "+filename
  os.unlink(filename)

def main():
  global verbose, atomic, base_dir, current_cvsroot, outfile, errfile
  try:
    opts, args = getopt.getopt(sys.argv[1:], "va", ["verbose", "atomic"])
  except getopt.GetoptError:
    usage()
    sys.exit(2)
  verbose = 0
  atomic = 0
  for o,a in opts:
    if o in ("-v", "--verbose"):
      verbose = 1
    if o in ("-a", "--atomic"):
      atomic = 1

  base_dir = os.getcwd()
  outfile = base_dir+'/testcvs.out'
  errfile = base_dir+'/testcvs.err'
  current_cvsroot = '/repos'
  current_tree = base_dir+'/tree'
  current_test = '(none)'
  test_data = base_dir+'/test_data'
  try:
    shutil.rmtree(base_dir+current_cvsroot,1) 
  except OSError:
    0
  try:
    shutil.rmtree(current_tree,1) 
  except OSError:
    0
  os.mkdir(base_dir+current_cvsroot)
  os.mkdir(current_tree)

  start_test('Basic functionality, Init, Import, Checkout')
  cvs_pass('-v')
  cvs_fail('version')
  cvs_pass('init')
  dir_exists(base_dir+current_cvsroot+'/CVSROOT')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/checkoutlist')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/checkoutlist,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/commitinfo')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/commitinfo,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/config')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/config,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/cvswrappers')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/cvswrappers,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/editinfo')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/editinfo,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/history')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/loginfo')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/loginfo,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/modules')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/modules,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/notify')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/notify,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/rcsinfo')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/rcsinfo,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/taginfo')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/taginfo,v')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/verifymsg')
  file_exists(base_dir+current_cvsroot+'/CVSROOT/verifymsg,v')
  # This should probably do the whole checkout/commit of cvsroot thing.
  if atomic:
    print 'Switching to atomic mode...'
    os.chmod(base_dir+current_cvsroot+'/CVSROOT/config',0644)
    file_copy(test_data + '/atomic_config', base_dir+current_cvsroot+'/CVSROOT/config')
  os.chdir(test_data + '/import_test')
  cvs_pass('import -m "Initial import" testcvs test1 test2')
  dir_exists(base_dir+current_cvsroot+'/testcvs')
  dir_exists(base_dir+current_cvsroot+'/testcvs/sub')
  dir_exists(base_dir+current_cvsroot+'/testcvs/sub2')
  file_exists(base_dir+current_cvsroot+'/testcvs/test1.txt,v')
  file_exists(base_dir+current_cvsroot+'/testcvs/test2.txt,v')
  file_exists(base_dir+current_cvsroot+'/testcvs/sub/test3.txt,v')
  file_exists(base_dir+current_cvsroot+'/testcvs/sub/test4.txt,v')
  file_exists(base_dir+current_cvsroot+'/testcvs/sub2/test5.txt,v')
  file_exists(base_dir+current_cvsroot+'/testcvs/sub2/test6.txt,v') 
  cvs_pass('version')
  os.chdir(current_tree)
  cvs_fail('co cvsfailtest')
  cvs_pass('co testcvs')
  dir_exists(current_tree+'/testcvs')
  dir_exists(current_tree+'/testcvs/CVS')
  dir_exists(current_tree+'/testcvs/sub')
  dir_exists(current_tree+'/testcvs/sub2')
  file_exists(current_tree+'/testcvs/test1.txt')
  file_exists(current_tree+'/testcvs/test2.txt')
  file_exists(current_tree+'/testcvs/sub/test3.txt')
  file_exists(current_tree+'/testcvs/sub/test4.txt')
  file_exists(current_tree+'/testcvs/sub2/test5.txt')
  file_exists(current_tree+'/testcvs/sub2/test6.txt')
  file_compare(test_data+'/import_test/test1.txt',current_tree+'/testcvs/test1.txt')
  file_compare(test_data+'/import_test/test2.txt',current_tree+'/testcvs/test2.txt')
  file_compare(test_data+'/import_test/sub/test3.txt',current_tree+'/testcvs/sub/test3.txt')
  file_compare(test_data+'/import_test/sub/test4.txt',current_tree+'/testcvs/sub/test4.txt')
  file_compare(test_data+'/import_test/sub2/test5.txt',current_tree+'/testcvs/sub2/test5.txt')
  file_compare(test_data+'/import_test/sub2/test6.txt',current_tree+'/testcvs/sub2/test6.txt')

  start_test('Basic Add, Remove, Resurrect, Commit')

  os.chdir(current_tree+'/testcvs')
  file_copy(test_data+'/add_test.txt','add_test.txt')
  cvs_pass('add add_test.txt')
  file_exists(current_tree+'/testcvs/add_test.txt')
  file_not_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  cvs_pass('commit -m "" add_test.txt')
  file_exists(current_tree+'/testcvs/add_test.txt')
  file_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  cvs_pass('remove -f add_test.txt')
  file_not_exists(current_tree+'/testcvs/add_test.txt')
  file_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  cvs_pass('add add_test.txt')
  file_exists(current_tree+'/testcvs/add_test.txt')
  file_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  cvs_pass('commit -m "" add_test.txt')
  cvs_pass('remove -f add_test.txt')
  file_not_exists(current_tree+'/testcvs/add_test.txt')
  file_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  cvs_pass('commit -m "" add_test.txt')
  file_not_exists(current_tree+'/testcvs/add_test.txt')
  file_not_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  dir_exists(base_dir+current_cvsroot+'/testcvs/Attic')
  file_exists(base_dir+current_cvsroot+'/testcvs/Attic/add_test.txt,v')
  file_copy(test_data+'/add_test.txt','add_test.txt')
  cvs_pass('add add_test.txt')
  cvs_pass('commit -m "" add_test.txt')
  file_exists(current_tree+'/testcvs/add_test.txt')
  file_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  cvs_pass('remove -f add_test.txt')
  file_not_exists(current_tree+'/testcvs/add_test.txt')
  cvs_pass('commit -m "" add_test.txt')
  file_not_exists(current_tree+'/testcvs/add_test.txt')
  file_not_exists(base_dir+current_cvsroot+'/testcvs/add_test.txt,v')
  dir_exists(base_dir+current_cvsroot+'/testcvs/Attic')
  file_exists(base_dir+current_cvsroot+'/testcvs/Attic/add_test.txt,v')
  cvs_pass('remove -f add_test.txt') # Should fail IMHO but standard cvs doesn't
  cvs_fail('commit -m "" fail_test.txt')

  start_test('Basic binary Add/Checkout')
  
  os.chdir(current_tree+'/testcvs')
  file_copy(test_data+'/binary_test.gif',current_tree+'/testcvs/binary_test.gif')
  cvs_pass('add -kb binary_test.gif')
  cvs_pass('commit -m "" binary_test.gif')
  file_exists(current_tree+'/testcvs/binary_test.gif')
  file_exists(base_dir+current_cvsroot+'/testcvs/binary_test.gif,v')
  file_compare(test_data+'/binary_test.gif',current_tree+'/testcvs/binary_test.gif')
  file_delete(current_tree+'/testcvs/binary_test.gif')
  cvs_pass('update binary_test.gif')
  file_compare(test_data+'/binary_test.gif',current_tree+'/testcvs/binary_test.gif')

  start_test('Add/Checkout large file')

  file_copy(test_data+'/maastrict.txt',current_tree+'/testcvs/maastrict.txt')
  cvs_pass('add maastrict.txt')
  cvs_pass('commit -m "" maastrict.txt')
  file_exists(current_tree+'/testcvs/maastrict.txt')
  file_exists(base_dir+current_cvsroot+'/testcvs/maastrict.txt,v')
  file_compare(test_data+'/maastrict.txt',current_tree+'/testcvs/maastrict.txt')
  file_delete(current_tree+'/testcvs/maastrict.txt')
  cvs_pass('update maastrict.txt')
  file_compare(test_data+'/maastrict.txt',current_tree+'/testcvs/maastrict.txt')

  start_test('Commit 50 revisions of a small file')
  for i in range(25):
#  start_test('Infinite commits of small file')
#  while(1):
    file_copy(test_data+'/diff_test.txt.1',current_tree+'/testcvs/test1.txt')
    cvs_pass('commit -f -m "" test1.txt')
    file_copy(test_data+'/diff_test.txt.2',current_tree+'/testcvs/test1.txt')
    cvs_pass('commit -f -m "" test1.txt')

  start_test('Commit 50 revisions of a large file')
  for i in range(25):
    file_copy(test_data+'/diff_test.txt.1',current_tree+'/testcvs/maastrict.txt')
    cvs_pass('commit -f -m "" maastrict.txt')
    file_copy(test_data+'/maastrict.txt',current_tree+'/testcvs/maastrict.txt')
    cvs_pass('commit -f -m "" maastrict.txt') 

  start_test('Checkout/Diff different versions of a file')

  os.chdir(current_tree+'/testcvs')
  file_copy(test_data+'/diff_test.txt.1',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('add diff_test.txt')
  cvs_pass('commit -m "" diff_test.txt')
  file_copy(test_data+'/diff_test.txt.2',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -f -m "" diff_test.txt')
  file_copy(test_data+'/diff_test.txt.3',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -f -m "" diff_test.txt')
  file_copy(test_data+'/diff_test.txt.4',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -f -m "" diff_test.txt')
  cvs_pass('update -r 1.1 diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_compare(test_data+'/diff_test.txt.1',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('update -r 1.2 diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_compare(test_data+'/diff_test.txt.2',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('update -r 1.3 diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_compare(test_data+'/diff_test.txt.3',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('update -r 1.4 diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_compare(test_data+'/diff_test.txt.4',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('update -r 1.1 diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_compare(test_data+'/diff_test.txt.1',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('update -A diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_compare(test_data+'/diff_test.txt.4',current_tree+'/testcvs/diff_test.txt')
  cvs_fail('-q diff -r 1.1 -r 1.3 diff_test.txt') # CVS always returns 1
  file_compare(outfile,test_data+'/diff_test.diff.1')
  cvs_fail('-q diff -r 1.3 -r 1.1 diff_test.txt') # CVS always returns 1
  file_compare(outfile,test_data+'/diff_test.diff.2')
  cvs_fail('-q diff -r 1.2 diff_test.txt') # CVS always returns 1
  file_compare(outfile,test_data+'/diff_test.diff.3')

  start_test('Branching')

  cvs_pass('update -r 1.3 diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  cvs_pass('tag -b branch_test diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  cvs_pass('update -r branch_test diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  file_copy(test_data+'/branch_test.txt.1',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -m "" diff_test.txt')
  file_copy(test_data+'/branch_test.txt.2',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -m "" diff_test.txt')
  cvs_pass('log -t diff_test.txt')
  file_compare(outfile,test_data+'/branch_test.txt.3')

  start_test('Merging')

  cvs_pass('update -A diff_test.txt')
  file_exists(current_tree+'/testcvs/diff_test.txt')
  cvs_fail('diff -r branch_test diff_test.txt')
  file_compare(outfile,test_data+'/merge_test.txt.1')
  cvs_pass('update -j branch_test diff_test.txt')
  file_compare(outfile,test_data+'/merge_test.txt.2')
  file_compare('diff_test.txt',test_data+'/merge_test.txt.3')
  file_copy(test_data+'/merge_test.txt.4',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -m "" diff_test.txt')
  cvs_pass('update -r branch_test diff_test.txt')
  file_copy(test_data+'/merge_test.txt.5',current_tree+'/testcvs/diff_test.txt')
  cvs_pass('commit -m "" diff_test.txt')
  cvs_pass('update -A diff_test.txt')
  cvs_pass('update -m -j branch_test diff_test.txt')
  file_compare('diff_test.txt',test_data+'/merge_test.txt.6')
  os.unlink('diff_test.txt')
  cvs_pass('update diff_test.txt')
  cvs_pass('update -b -j branch_test diff_test.txt')
  file_compare('diff_test.txt',test_data+'/merge_test.txt.7')

  start_test('*info')

  os.chmod(base_dir+current_cvsroot+'/CVSROOT/commitinfo',0644)
  os.chmod(base_dir+current_cvsroot+'/CVSROOT/loginfo',0644)
  os.chmod(base_dir+current_cvsroot+'/CVSROOT/postcommit',0644)
  file_copy(test_data + '/commitinfo_test', base_dir+current_cvsroot+'/CVSROOT/commitinfo')
  file_copy(test_data + '/loginfo_test', base_dir+current_cvsroot+'/CVSROOT/loginfo')
  file_copy(test_data + '/postcommit_test', base_dir+current_cvsroot+'/CVSROOT/postcommit')
  cvs_pass('commit -f -m "info test" diff_test.txt')
  if os.name == 'nt':
    file_compare(outfile, test_data+'/info_test_output.w32')
  else:
    file_compare(outfile, test_data+'/info_test_output.txt')

if __name__ == "__main__":
  main()

