from __future__ import print_function

import optparse
import os
import shutil

import irods_python_ci_utilities

def install_r_packages():
    install_testing_dependencies()
    irods_python_ci_utilities.install_os_packages(['r-base'])
    # copy source into tmp dir, build it, install it as irods user
    build_dir = os.getcwd()
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'chmod', '-R', '777', build_dir], check_rc=True)
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'su', '-', 'irods', '-c', 'export R_LIBS={0} && cd {0} && R CMD BATCH install-Rcpp.R'.format(build_dir)], check_rc=True)
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'su', '-', 'irods', '-c', 'export R_LIBS={0} && cd {0} && make r_cmd_pkg'.format(build_dir)], check_rc=True)
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'su', '-', 'irods', '-c', 'export R_LIBS={0} && cd {0} && R CMD BATCH install-packages.R'.format(build_dir)], check_rc=True)
    # sync test files
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'su', '-', 'irods', '-c', 'rsync -r {0}/tests/ /var/lib/irods/tests/pydevtest/'.format(build_dir)], check_rc=True)

def run_tests():
#    install_testing_dependencies()
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'su', '-', 'irods', '-c', 'export R_LIBS={0} && cd /var/lib/irods/tests/pydevtest && python run_tests.py --xml_output --run_specific_test test_client_rirods'.format(os.getcwd())], check_rc=False)

def install_testing_dependencies():
    dispatch_map = {
        'Ubuntu': install_testing_dependencies_apt,
    }
    try:
        return dispatch_map[irods_python_ci_utilities.get_distribution()]()
    except KeyError:
        irods_python_ci_utilities.raise_not_implemented_for_distribution()

def install_testing_dependencies_apt():
    irods_python_ci_utilities.install_os_packages(['libjansson4','libkrb5-dev'])

def gather_xml_reports(output_root_directory):
    shutil.copytree('/var/lib/irods/tests/pydevtest/test-reports', os.path.join(output_root_directory, 'test-reports'))

def gather_logs(output_root_directory):
    irods_python_ci_utilities.subprocess_get_output(['sudo', 'chmod', '-R', '777', '/var/lib/irods'], check_rc=True)
    irods_python_ci_utilities.subprocess_get_output(['rsync', '-r', '/var/lib/irods/iRODS/server/log/', output_root_directory+'/'], check_rc=True)

def main(output_root_directory):
    install_r_packages()
    run_tests()
    if output_root_directory:
        irods_python_ci_utilities.mkdir_p(output_root_directory)
        gather_xml_reports(output_root_directory)
        gather_logs(output_root_directory)

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('--output_root_directory')
    options, _ = parser.parse_args()

    main(options.output_root_directory)
