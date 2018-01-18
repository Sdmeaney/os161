set nocompatible                " choose no compatibility with legacy vi
syntax enable
set encoding=utf-8
set showcmd                     " display incomplete commands
filetype plugin indent on       " load file type plugins + indentation

set grepprg=grep\ -nH\ $*
let g:tex_flavor='latex'
let g:Tex_DefaultTargetFormat='pdf'
"set runtimepath=~/.vim,$VIM/vimfiles,$VIMRUNTIME,$VIM/vimfiles/after,~/.vim/after

"" Wrapping
set wrap
set linebreak
set nolist

"" Make scrolling through wrapped lines easier
imap <silent> <Down> <C-o>gj
imap <silent> <Up> <C-o>gk
nmap <silent> <Down> gj
nmap <silent> <Up> gk

"" Whitespace
set tabstop=3 shiftwidth=3      " tab is tree spaces
set backspace=indent,eol,start  " backspace through everything in insert mode

"" Searching
set hlsearch                    " highlighting matches
set incsearch                   " incremental searching
set ignorecase                  " searches are case insensitive...
set smartcase                   " ... unless they contain at least one capital letter
"" Color Scheme
colorscheme desert              " Makes the comments easier to read

set nonumber
