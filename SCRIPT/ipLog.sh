#!/bin/bash

touch /home/joseluis/practicaScript/ficheroIP

ESIP='[0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}'

echo "El script $0 está siendo ejecutado por $USERNAME, al momento de `date`"
echo "La versión de bash utilizada es $BASH_VERSION"



# con 0 parametros buscar ultimas 100 lineas fichero /var/log/auth.log cual es la ip que mas aparece y el numero de veces

if test $# -eq 0
then
	
	tail -n 100 /home/joseluis/practicaScript/Ejemplo_auth.log | grep -E -o '[0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}' > /home/joseluis/practicaScript/ficheroIP
	
	
	contadorMayor=0
	contadorAux=0
	ipAux=0\.0\.0\.0
	
	
	for IP in $(tail -n 100 /home/joseluis/practicaScript/Ejemplo_auth.log | grep -E -o '[0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}')
	do
		contadorAux=$(grep -o $IP /home/joseluis/practicaScript/ficheroIP | wc -l | cut -d ' ' -f 1)
		

		if test $contadorAux -gt $contadorMayor
		then
			contadorMayor=$contadorAux
			ipAux=$IP	
		fi 
	done
	
	echo "La ip mas repetida es $ipAux con $contadorMayor veces"



# con 1 parametro, si es una IP busca esa ip en el /var/log/auth.log y se indica el numero de veces que aparece
# con 1 parametro, si es directorio, buscamos los .log contenidos la ultima ip que aparece en el /var/log/auth.log

elif test $# -eq 1
then 
	
	if test -d $1
	then
		
		IP=$(grep -E -o '[0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}[\./\-][0-9]{1,3}' /home/joseluis/practicaScript/Ejemplo_auth.log | tail -n 1 )
		
		for FILE in $(find $1 -name "*.log")
		do
			grep -H -o $IP $FILE
		done

	elif [[ $1 =~ $ESIP ]]
	then
		grep -E -o $1 /home/joseluis/practicaScript/Ejemplo_auth.log | wc -l
	
	else

		echo "Error: El directorio o la ip no existen"
		rm /home/joseluis/practicaScript/ficheroIP 
		exit 3
	fi



# con 2 parametros, siendo el 1 el path y el 2 la IP, buscamos la IP dentro de la ruta indicada

elif test $# -eq 2
then
	if test -d $1 && [[ $2 =~ $ESIP ]]
	then

		for FILE in $(find $1 -name "*.log")
		do

			grep -H $2 $FILE

		done
	else
		echo "Error: No existen los argumentos u orden incorrecto de argumentos"
		rm /home/joseluis/practicaScript/ficheroIP 
		exit 2
	fi
	
else 
# si el numero de argumentos es mayor de 2
	echo "Error: No has introducido bien los argumentos en $0"
	rm /home/joseluis/practicaScript/ficheroIP 
	exit 1
fi


rm /home/joseluis/practicaScript/ficheroIP 




