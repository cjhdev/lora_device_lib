#!/usr/bin/env bash

# this will generate a set of key-pairs plus a specially formatted environment variable for Travis

# generate(name)
function generate()
{
    mkdir -p secrets/$1
    ssh-keygen -b 2048 -t rsa -f secrets/$1/id_rsa -q -N ""    
    cat secrets/$1/id_rsa | sed 's/ /\\ /g' | tr '\n' '.' > secrets/$1/id_rsa_env
}

rm -rf secrets

generate api
