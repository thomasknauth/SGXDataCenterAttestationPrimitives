# This file is provided under a dual BSD/GPLv2 license.  When using or
# redistributing this file, you may do so under either license.
#
# GPL LICENSE SUMMARY
#
# Copyright(c) 2016-2018 Intel Corporation.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# Contact Information:
# Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>
# Intel Finland Oy - BIC 0357606-4 - Westendinkatu 7, 02160 Espoo
#
# BSD LICENSE
#
# Copyright(c) 2016-2018 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors:
#
# Jarkko Sakkinen <jarkko.sakkinen@linux.intel.com>

KASAN_SANITIZE := n
OBJECT_FILES_NON_STANDARD := y
KCOV_INSTRUMENT := n
KBUILD_CFLAGS := -Wall -Werror -static -nostdlib -nostartfiles -fno-builtin \
		 -I$(obj)/include -I$(PWD)/include
KBUILD_AFLAGS += -I$(obj)/include -I$(PWD)/include

subdir- := enclave

always := sgx_le_proxy
clean-files := sgx_le_proxy

#
# sgx_le_proxy
#

sgx_le_proxy-y += main.o entry.o string.o
targets += $(sgx_le_proxy-y)
SGX_LE_PROXY_OBJS = $(addprefix $(obj)/,$(sgx_le_proxy-y))

$(obj)/main.o: $(obj)/enclave/sgx_le_blob.h $(obj)/enclave/sgx_le_ss.h

$(obj)/enclave/sgx_le_blob.h $(obj)/enclave/sgx_le_ss.h $(SIGNING_MATERIAL): 
	$(Q)$(MAKE) $(build)=$(obj)/enclave $@

targets += sgx_le_proxy
$(obj)/sgx_le_proxy: $(SGX_LE_PROXY_OBJS)
	$(call if_changed,ld)

clean:
	rm -vrf *.o .*.cmd $(clean-files)
# "make clean" is cleaning also the LE including the signed pre-built sgx_le_blob.h and sgx_le_ss.h.
# to avoid cleaning them we can use the flag below and call "make clean SKIP_LE_CLEAN=1" - in this 
# case the LE will not be cleaned and won't be rebuilt. 
ifneq ($(SKIP_LE_CLEAN),1)
	$(Q)$(MAKE) -C enclave clean
endif

