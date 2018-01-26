# StarPU --- Runtime system for heterogeneous multicore architectures.
#
# Copyright (C) 2012                                     Inria
# Copyright (C) 2010-2012,2015,2017,2018                 CNRS
# Copyright (C) 2010,2014                                Université de Bordeaux
#
# StarPU is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#
# StarPU is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU Lesser General Public License in COPYING.LGPL for more details.
#
find . -type f -not -name rename.sed |xargs sed -i -f $(dirname $0)/rename.sed
