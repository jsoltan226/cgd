#!/bin/sh

error() {
    printf "[31;1mERROR[0m: %s. Stop.\n" "$1" >&2
    exit "$2"
}

warn() {
    printf "[33;1mWARNING[0m: %s.\n" "$1" >&2
}

[ -z "$CC" ] && CC=cc
[ -z "$CCLD" ] && CCLD="$CC"
[ -z "$CXX" ] && CXX=c++
[ -z "$CXXLD" ] && CXXLD="$CXX"

[ -z "$OBJDIR" ] && OBJDIR='./obj'
[ -z "$BINDIR" ] && BINDIR='./bin'

no_c_sources=1
no_cpp_sources=1

C_SRCS="$(find . -type f -name "*.c")"
[ -n "$C_SRCS" ] && no_c_sources=0
CXX_SRCS="$(find . -type f -name "*.cpp")"
[ -n "$CXX_SRCS" ] && no_cpp_sources=0

[ $no_c_sources -eq 1 ] && [ $no_cpp_sources -eq 1 ] &&
    error "No source files found" 1

[ -d "$OBJDIR" ] || {
    warn "OBJDIR is missing. Creating.."
    mkdir "$OBJDIR" ||
        error "Failed to create $OBJDIR directory" 2
}

_sed_args="s/.*\/\(.*\)/&\\ $(echo "$OBJDIR" | sed -e "s/\\//\\\\\//g")\/\1.o\\ $(echo "$OBJDIR" | sed -e "s/\\//\\\\\//g")\/\1.d/"

C_SRC_OBJ_DEP_TRIPLES="$(echo "$C_SRCS" | sed -e "$_sed_args")"
CXX_SRC_OBJ_DEP_TRIPLES="$(echo "$C_SRCS" | sed -e "$_sed_args")"

i=0
srcfile=
objfile=
depfile=
for f in $C_SRC_OBJ_DEP_TRIPLES; do
    i=$((i + 1))
    case "$i" in
        "1") srcfile="$f" ;;
        "2") objfile="$f" ;;
        "3") depfile="$f" 
            
            [ -f "$depfile" ] || $CC -E -MMD -MP -MF "$depfile" "$srcfile" >/dev/null
            cat "$depfile"
            printf "\033[1mCC\033[0m\t%s <= %s\n" "$objfile" "$srcfile"
            $CC -o "$objfile" "$srcfile" -c

            unset srcfile
            unset objfile
            unset depfile
            i=0
            continue;
            ;;
    esac
done
