#!/bin/bash
# Lab 2: contacts.sh - grep usage example

CONTACTS=contacts.txt

PS3="Select an option: "

select task in "Show All Information" "Show Phone" "Show Email" "Show Web"
do
  name=""
  if [ "$REPLY" -le "4" ] && [ "$REPLY" -ge "1" ]; then
    while [ -z "$name" ]
    do
      read -p "Enter a name to search for: " name
    done
    case $REPLY in
      1) 
	  grep -A3 -i "^Name:.*${name}" $CONTACTS
	  ;;
      2)
	  grep -A1 -i "^Name:.*${name}" $CONTACTS | cut -d: -f2-
          ;;
      3)
          grep -A2 -i "^Name:.*${name}" $CONTACTS | \
          grep -e "^Email:" -e "^Name:"| cut -d: -f2-
          ;;
      4)
	  grep -A3 -i "^Name:.*${name}" $CONTACTS | \
          grep -e "^Web:" -e "^Name:"| cut -d: -f2-
          ;;
    esac
  fi
done
