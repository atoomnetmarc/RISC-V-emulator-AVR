Import("env", "projenv")
from shutil import copyfile

def copyhex(*args, **kwargs):
    copyfile(str(kwargs['target'][0]), 'hex/'+env['PIOENV']+'.hex')

env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", copyhex)
