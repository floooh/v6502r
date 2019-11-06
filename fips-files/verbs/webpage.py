import os, yaml, shutil, subprocess, glob
from mod import log, util, project

BuildConfig = 'wasm-ninja-release'

#-------------------------------------------------------------------------------
def build_deploy_webpage(fips_dir, proj_dir):
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/v6502r-webpage'.format(ws_dir)
    if not os.path.isdir(webpage_dir) :
        os.makedirs(webpage_dir)
    
    project.gen(fips_dir, proj_dir, BuildConfig)
    project.build(fips_dir, proj_dir, BuildConfig)
    
    src_dir = '{}/fips-deploy/v6502r/{}'.format(ws_dir, BuildConfig)
    dst_dir = webpage_dir

    shutil.copy(src_dir+'/v6502r.html', dst_dir+'/index.html')
    shutil.copy(src_dir+'/v6502r.wasm', dst_dir+'/v6502r.wasm')
    shutil.copy(src_dir+'/v6502r.js', dst_dir+'/v6502r.js')

    log.colored(log.GREEN, 'Generated Samples web page under {}.'.format(webpage_dir))

#-------------------------------------------------------------------------------
def serve_webpage(fips_dir, proj_dir) :
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/v6502r-webpage'.format(ws_dir)
    p = util.get_host_platform()
    if p == 'osx' :
        try :
            subprocess.call(
                'open http://localhost:8000 ; python {}/mod/httpserver.py'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt :
            pass
    elif p == 'win':
        try:
            subprocess.call(
                'cmd /c start http://localhost:8000 && python {}/mod/httpserver.py'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass
    elif p == 'linux':
        try:
            subprocess.call(
                'xdg-open http://localhost:8000; python {}/mod/httpserver.py'.format(fips_dir),
                cwd = webpage_dir, shell=True)
        except KeyboardInterrupt:
            pass

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    if len(args) > 0 :
        if args[0] == 'build' :
            build_deploy_webpage(fips_dir, proj_dir)
        elif args[0] == 'serve' :
            serve_webpage(fips_dir, proj_dir)
        else :
            log.error("Invalid param '{}', expected 'build' or 'serve'".format(args[0]))
    else :
        log.error("Param 'build' or 'serve' expected")

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
             'fips webpage build\n' +
             'fips webpage serve\n' +
             log.DEF +
             '    build the visual6502remix webpage')
