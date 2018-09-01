#!/bin/bash

############################# FUNCTIONS #############################

# func: isEmpty - check variable for a null (""), 0, 0.0 values
# param: $value
function isEmpty(){
    local var="$1"

    # Return true if:
    # 1.    var is a null string ("" as empty string)
    # 2.    a non set variable is passed
    # 3.    a declared variable or array but without a value is passed
    # 4.    an empty array is passed
    if test -z "$var"
    then
        [[ $( echo "1" ) ]]
        return

    # Return true if var is zero (0 as an integer or "0" as a string)
    elif [ "$var" == 0 2> /dev/null ]
    then
        [[ $( echo "1" ) ]]
        return

    # Return true if var is 0.0 (0 as a float)
    elif [ "$var" == 0.0 2> /dev/null ]
    then
        [[ $( echo "1" ) ]]
        return
    fi

    [[ $( echo "" ) ]]
}


# func: WriteFile - creates files in entered or default path with num copies 
# param: $path - Path for created file to fill
function WriteFile() {

if ! [ -f $1 ] ; then		#if file not exist - ERROR

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "ПОМИЛКА: файл не існує!" >&2
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "ОШИБКА: файл не существует!" >&2
else
	echo "ERROR: file not exist!" >&2
fi

exit -1
fi

################ Date
echo "Date: " $(date -R) >> $1
echo "---- Hardware ----" >> $1
################

################ CPU
CPU="$(grep -m 1 "model name" /proc/cpuinfo)"
echo "CPU: \"${CPU##*: }\"" >> $1
################

################ RAM v1
#RAM="$(grep -m 1 "MemTotal" /proc/meminfo | awk '{print $2}')"
#RAM="$(expr $RAM / 1024)"
#echo "RAM: $RAM MB" >> $1

################ RAM v2
unset RAMarray
RAMarray=( $(sudo dmidecode -t memory | grep Size | awk '{print $2}') )
RAM=0
for i in "${RAMarray[@]}" ;
do 
if [[ "$i" -ne "No" ]] ; then
RAM="$(expr $RAM + $i)"
fi
done
echo "RAM: ${RAM} MB" >> $1
################


################ MOTHERBOARD
Manufact="$(sudo dmidecode -t 2 | grep "Manufacturer")"
ProductName="$(sudo dmidecode -t 2 | grep "Product Name")"
SN="$(sudo dmidecode -t 2 | grep "Serial Number")"
Manufact="${Manufact##*: }"
ProductName="${ProductName##*: }"
SN="${SN##*: }"

if [ -z "$Manufact" ] ; then
Manufact="Unknown"
fi
if [ -z "$ProductName" ] ; then
ProductName="Unknown"
fi
if [ -z "$SN" ] ; then
SN="Unknown"
fi

echo "Motherboard: \"$Manufact\", \"$ProductName\"" >> $1
echo "Serial System Number: $SN" >> $1
################ 

echo "---- System ----" >> $1
################ OS
Distrib="$(grep "DISTRIB_DESCRIPTION" /etc/*-release)"
Distrib="${Distrib##*=}"

if (isEmpty $Distrib) ; then
Distrib="$(grep "PRETTY_NAME" /etc/*-release)"
Distrib="${Distrib##*=}"
fi
if (isEmpty $Distrib) ; then
Distrib="Unknow"
fi
echo "OS Destribution: $Distrib" >> $1

Kernel="$(uname -mrs)"
Kernel=${Kernel:6}
if (isEmpty $Kernel) ; then 
$Kernel="Unknown" 
fi
echo "Kernel version: $Kernel" >> $1

InstDate="$(uname -a |tail -1|awk '{print $6, $7, $8, $9, $11}')"
if (isEmpty $InstDate) ; then 
$InstDate="Unknown"
fi
echo "Installation date: $InstDate" >> $1

host="$(hostname -I | awk '{print $1}')"
if (isEmpty $host) ; then 
$host="Unknown"
fi
echo "Hostname: $host" >> $1

Uptime="$(uptime | awk '{print $3}')"
Uptime=${Uptime%%,}
if (isEmpty $Uptime) ; then 
$Uptime="Unknown"
fi
echo "Uptime: $Uptime" >> $1

processes="$(ps aux --no-heading | wc -l)"
if (isEmpty $processes) ; then 
$processes="Unknown"
fi
echo "Processes running: $processes" >> $1

users="$(uptime | awk '{print $5}')"
users=${users## }
if (isEmpty $users) ; then 
$users="Unknown"
fi
echo "User logged in: $users" >> $1
################


################ NETWORK
echo "---- Network ----" >> $1

echo "$(ip -o -4 a | cut -d " " -f2,7)" >> $1

echo "----\"EOF\"----" >> $1
################

} #end WriteFile


# func: help - prints help in terminal
function help(){

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "Використовування: ./task1.sh [-h|--help] [-n num] [file]" 
	echo "num -- кількість файлів із результатами,"
	echo "file -- шлях та ім'я файла, у який треба записати результат;"
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "Использование: ./task1.sh [-h|--help] [-n num] [file]" 
	echo "num -- количество файлов с результатами,"
	echo "file -- путь и имя файла, в который необходимо записать результат;"
else
	echo "Usage: ./task1.sh [-h|--help] [-n num] [file]" 
	echo "num -- files count with results,"
	echo "file -- path and file name to write result;"
fi


}
############################# FUNCTIONS #############################



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

############# num & path
-n)
shift

re='^[0-9]+$'
if ! [[ $1 =~ $re && $1 -gt 0 && $1 -le 1000 ]];	# check for integer val & var > 0
then

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "ПОМИЛКА: 'num' не може бути менше 1 або не цілим числом!" >&2
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "ОШИБКА: 'num' не может быть меньше 1 или не целым числом!" >&2
else
	echo "ERROR: 'num' cannot be less then 1 or floating value!" >&2
fi
exit 1
fi

num_files=$1
;;
############# num & path

*)
path=$1  					#here may be a path
if [ -z "$path" ] ; then 			#if path NULL
path="bash/task1.out"
cd ~						#go home folder
shift
fi
;;

############# default (another) *)
#if [ "$LANG" = "uk_UA.UTF-8" ] ; then
#	echo "ПОМИЛКА: не корректні параметри!" >&2
#elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
#	echo "ОШИБКА: не корректные параметры!" >&2
#else
#	echo "ERROR: incorrect parameters!" >&2
#fi
#exit 2
#;;

############# default (another)

esac	#end case

shift
done	#end while

#############################################################################


if (isEmpty $path) ; then
path="bash/task1.out"				#default path
cd ~						#go home folder
fi

dir_path=$(dirname $path)

if ! mkdir -p $dir_path ; then			#creating folders if it not exist

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "ПОМИЛКА: не вдалося створити папку!" >&2
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "ОШИБКА: не удалось создать папку!" >&2
else
	echo "ERROR: could not create folder!" >&2
fi

exit 3
fi

filename="${path##*/}"
filename="${filename%.*}"		#get file name
extension="${path##*.}"			#get file extension

n=$(printf "%04d" 0)

cd $dir_path
oldname="$filename.$extension"
newname=$oldname

while (( 1 ))
do
if [ -f "$newname" ] ; then			#check if file exist
	newname="$filename-$(date +%Y%m%d)-$n.$extension"
	n="$(expr $n + 1)"
	n=$(printf "%04d" $n)
else
if [[ "$oldname" != "$newname" ]] ; then #if file not exist yet (oldname == newname, dir is empty)
		mv $oldname $newname 		#rename the file
	fi
	break;
fi
done


if ! touch $oldname; then			#creating file

if [ "$LANG" = "uk_UA.UTF-8" ] ; then
	echo "ПОМИЛКА: не вдалося створити файл!" >&2
elif [[ "$LANG" = "ru_RU.UTF-8" || "$LANG" = "ru_UA.UTF-8" ]] ; then
	echo "ОШИБКА: не удалось создать файл!" >&2
else
	echo "ERROR: could not create file!" >&2
fi

exit 4
fi


#fill file with sys info
(WriteFile $oldname)

# delete n files (if entered)
FileList=( $(ls) ) 	#get file list
len="${#FileList[*]}"
if [[ $num_files && $num_files -le $len ]] ; then
del="$(expr $len - $num_files)"
echo "Deleting $del files"
for (( i=0; i < del ; i++ ))
do
rm "${FileList[i]}"
done
fi


#if [ -d "$path" ]
#then
#echo "$path is a directory"
#elif [ -f "$path" ]
#then
#echo "$path is a file"
#fi

#if [ -n $path]		#if path is not NULL
#delete prev output files
#create new files
#else			#if path NULL
			#create task1.out
#fi


#rm -rf mydir		#delete all files in directory
#rm file*		#delete all files with beginning "file"
#rm file[1-8]		#delete files with endian file1...file8
#rm $path		#delete one file

#: << 'END'

#END

cd ~

exit 0
