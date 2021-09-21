import os, yaml, shutil, subprocess, glob
from mod import log, util, project

BuildConfig = 'wasm-ninja-release'

#-------------------------------------------------------------------------------
def build_deploy_webpage(fips_dir, proj_dir):
    ws_dir = util.get_workspace_dir(fips_dir)
    z80_webpage_dir = '{}/fips-deploy/vz80r-webpage'.format(ws_dir)
    if not os.path.isdir(z80_webpage_dir) :
        os.makedirs(z80_webpage_dir)

    project.gen(fips_dir, proj_dir, BuildConfig)
    project.build(fips_dir, proj_dir, BuildConfig)

    for tgt in ['v6502r', 'vz80r']:
        webpage_dir = '{}/fips-deploy/{}-webpage'.format(ws_dir, tgt)
        if not os.path.isdir(webpage_dir) :
            os.makedirs(webpage_dir)

        src_dir = '{}/fips-deploy/v6502r/{}'.format(ws_dir, BuildConfig)
        dst_dir = webpage_dir

        shutil.copy(src_dir+'/{}.html'.format(tgt), dst_dir+'/index.html')
        shutil.copy(src_dir+'/{}.wasm'.format(tgt), dst_dir+'/{}.wasm'.format(tgt))
        shutil.copy(src_dir+'/{}.js'.format(tgt), dst_dir+'/{}.js'.format(tgt))

        log.colored(log.GREEN, 'Generated {} web page under {}.'.format(tgt, webpage_dir))

#-------------------------------------------------------------------------------
def serve_webpage(fips_dir, proj_dir, tgt) :
    ws_dir = util.get_workspace_dir(fips_dir)
    webpage_dir = '{}/fips-deploy/{}-webpage'.format(ws_dir, tgt)
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
            serve_webpage(fips_dir, proj_dir, args[1])
        else :
            log.error("Invalid param '{}', expected 'build' or 'serve'".format(args[0]))
    else :
        log.error("Param 'build' or 'serve' expected")

#-------------------------------------------------------------------------------
def help() :
    log.info(log.YELLOW +
             'fips webpage build\n' +
             'fips webpage serve [v6502r|vz80r]\n' +
             log.DEF +
             '    build the visual6502remix webpage')
