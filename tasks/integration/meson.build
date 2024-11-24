cppnet_proj = subproject('cppnet')
cppnet_dep = cppnet_proj.get_variable('cppnet_dep')

worker_sources = files('integral.cpp', 'random_loss.cpp', 'worker.cpp')
master_sources = files('random_loss.cpp', 'master.cpp')

executable('worker', worker_sources, dependencies: cppnet_dep)
executable('master', master_sources, dependencies: cppnet_dep)
