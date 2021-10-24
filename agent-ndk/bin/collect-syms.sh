#!/bin/sh

##
# Requires:
# Current NDK installed
# 
# ANDROID_NDK refers to NDK root directory

source $(dirname $0)/ndk.sh

OPTS=${OPTS:-"--strip-debug "}

echo "Updating archive: ${ARCHIVE}"
for ABI in ${ABIS} ; do
	OBJDIR="$BUILDDIR/intermediates/cmake/${VARIANT}/obj/${ABI}"
	NDK_OBJCOPY=$(ndkrequire "$ABI" "objcopy") || continue
	SYMS=$(find ${OBJDIR} -name "*.so" 2> /dev/null) 
	[[ -n "$SYMS" ]] || echo "No syms found in [$OBJDIR]" 
	for	SYM in $SYMS ; do
		OUT="$(basename $SYM).sym"		
		dbug "Collect unstripped symbols from [$SYM] and save to [$OUT]:"
		$NDK_OBJCOPY ${OPTS} $SYM "$OBJDIR/$OUT" || echo Error: objcopy failed
	done
	find ${OBJDIR} -name "*.sym" -exec zip -m "${ARCHIVE}" {} \;
done

echo
[[ -s "${ARCHIVE}" ]] || fatal "Error: archive is empty!"
echo "Final sym archive is:" 
ls -aFl ${ARCHIVE}
unzip -v ${ARCHIVE}

# Stripped already here: ./build/intermediates/stripped_native_libs/release/out/lib/x86/