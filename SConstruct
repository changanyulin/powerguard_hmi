import os

AWTK_ROOT = ARGUMENTS.get('AWTK_ROOT', r'D:\dev\awtk')
AWTK_ROOT = os.path.abspath(AWTK_ROOT)

BIN_DIR = os.path.join('.', 'bin')
BUILD_DIR = os.path.join('.', 'build')

CPPPATH = [
  AWTK_ROOT,
  os.path.join(AWTK_ROOT, 'src'),
  os.path.join(AWTK_ROOT, 'src', 'ext_widgets'),
  os.path.join(AWTK_ROOT, '3rd'),
  os.path.join(AWTK_ROOT, '3rd', 'SDL', 'include'),
  os.path.join(AWTK_ROOT, '3rd', 'gpinyin', 'include'),
  os.path.join(AWTK_ROOT, '3rd', 'mbedtls', 'include'),
  os.path.join(AWTK_ROOT, '3rd', 'libunibreak'),
  os.path.join(AWTK_ROOT, 'res'),
  os.path.join('.', 'src'),
]

LIBS = [
  'assets',
  'awtk',
  'tkc',
  'gdi32',
  'user32',
  'winmm',
  'imm32',
  'version',
  'shell32',
  'Setupapi',
  'ole32',
  'Oleaut32',
  'Advapi32',
  'DelayImp',
  'psapi',
  'ws2_32',
]

env = Environment(
  CCFLAGS=[
    '/DWIN32',
    '/DWINDOWS',
    '/D_CONSOLE',
    '/D_CRT_SECURE_NO_WARNINGS',
    '/D_HAS_EXCEPTIONS=0',
    '/D_HAS_ITERATOR_DEBUGGING=0',
    '/D_ITERATOR_DEBUG_LEVEL=0',
    '/DWITH_64BIT_CPU',
    '/DHAVE_LIBC',
    '/DHAS_STDIO',
    '/DHAVE_STDIO_H',
    '/DWITH_FS_RES',
    '/DWITH_SDL',
    '/DWITH_VGCANVAS',
    '/DWITH_DESKTOP_STYLE',
    '/DENABLE_CURSOR=1',
    '/MDd',
    '/D_DEBUG',
    '/DDEBUG',
    '/FS',
    '/Z7',
    '/utf-8',
  ],
  CPPPATH=CPPPATH,
  LIBPATH=[os.path.join(AWTK_ROOT, 'bin'), os.path.join(AWTK_ROOT, 'lib')],
  LIBS=LIBS,
  LINKFLAGS=['/SUBSYSTEM:WINDOWS', '/MACHINE:X64', '/DEBUG'],
  TARGET_ARCH='x86_64',
)

sources = [
  'src/app.c',
  'src/battery_model.c',
  'src/simulator.c',
  'src/alarm_manager.c',
  'src/config_store.c',
  'src/ui_controller.c',
]

objects = []
for source in sources:
  name = os.path.splitext(os.path.basename(source))[0]
  objects.append(env.Object(os.path.join(BUILD_DIR, name), source))

target = os.path.join(BIN_DIR, 'powerguard_hmi')
program = env.Program(target, objects)

awtk_dll = env.Command(os.path.join(BIN_DIR, 'awtk.dll'), os.path.join(AWTK_ROOT, 'bin', 'awtk.dll'),
                       Copy('$TARGET', '$SOURCE'))
tkc_dll = env.Command(os.path.join(BIN_DIR, 'tkc.dll'), os.path.join(AWTK_ROOT, 'bin', 'tkc.dll'),
                      Copy('$TARGET', '$SOURCE'))

Default(program, awtk_dll, tkc_dll)
