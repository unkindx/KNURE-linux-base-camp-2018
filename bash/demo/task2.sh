#!/bin/bash


# func: help - prints help in terminal
function help(){

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "Використовування: ./task2.sh [-h|--help]" 
	echo "Даний скрипт встановлює task1.sh в систему."
	
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "Использование: ./task2.sh [-h|--help]" 
	echo "Данный скрипт устанавливает task1.sh в систему."
else
	echo "Usage: ./task2.sh [-h|--help]" 
	echo "This script install task1.sh into system."
fi
}



############################# PARAMS PARSE #############################

while [ -n "$1" ]
do

case "$1" in

############# help
-h) 
help
exit 0 
;;
--help)
help
exit 0
;;
############# help

############# default
*)
if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "ПОМИЛКА: не корректні параметри!" >&2
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "ОШИБКА: не корректные параметры!" >&2
else
	echo "ERROR: incorrect parameters!" >&2
fi
exit 2
;;

esac	#end case

shift
done	#end while

#############################################################################


src_path="$HOME/task1/task1.sh"
dest_path="/usr/local/bin"

if ! ( sudo cp $src_path $dest_path ) ; then

if ! mkdir -p $dest_path ; then			#creating folders if it not exist

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "ПОМИЛКА: не вдалося створити папку!" >&2
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "ОШИБКА: не удалось создать папку!" >&2
else
	echo "ERROR: could not create folder!" >&2
fi

exit 3
fi

(sudo cp $src_path $dest_path)
fi

echo "File was installed to $dest_path"

exit 0


