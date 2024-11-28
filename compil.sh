#!/bin/bash
# Vérification sur le renseignement du chemin d'accès du dossier en argument
if [ $# -eq 0 ]; then
  echo "Please specify a directory as an argument."
  exit 1
fi
# Définition des fichiers de l'environnement de compilation
working_dir="$1"
tmp_folder="$working_dir/.tmp"
build_folder="$working_dir/build"
debug_folder="$working_dir/debug"

# Chemin d'accès à arduino.h
arduino_library_path="/home/mathieu-collin/Desktop/test_compilateur/Compilateur/Arduino15/packages/arduino/hardware/avr/1.8.6/cores/arduino/"

# Création des dossiers de l'environnement de travail
if [ ! -d "$tmp_folder" ]; then
  mkdir "$tmp_folder"
else
  rm -f "$tmp_folder"/*
fi
if [ ! -d "$build_folder" ]; then
  mkdir "$build_folder"
else
  rm -f "$build_folder"/*
fi
echo "COMPILATEUR:"

# Compilation des .cpp en .o
for source_file in "$working_dir"/*.cpp; do
  object_file="$build_folder/$(basename "$source_file" .cpp).o"
  avr-g++ -c -Os -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=22 -I "$arduino_library_path" -o "$object_file" "$source_file"
done

# Edition des liens
avr-gcc -mmcu=atmega328p -I "$arduino_library_path" -L "$arduino_library_path" -o "$working_dir/programme.elf" "$build_folder"/*.o -Wl,--start-group -larduino -Wl,--end-group

# Affichage des usages mémoire
memory_info=$(avr-size --mcu=atmega328p "$working_dir/programme.elf")

# Affichage des usages mémoire
flash_size=$(echo "$memory_info" | awk 'NR==2{print $1}')
ram_size=$(echo "$memory_info" | awk 'NR==2{print $2}')

echo "Memory Usage :"
echo "    -- Flash memory used: $flash_size bytes"
echo "    -- RAM memory used: $ram_size bytes"
echo ""
# Création du .hex à partir du .elf 
avr-objcopy -j .text -j .data -O ihex "$working_dir/programme.elf" "$working_dir/programme.hex"

# Téléversement du programme
read -p "Do you want to upload the program to the Arduino (Y/N)? " choice
if [ "$choice" = "Y" ] || [ "$choice" = "y" ]; then
  read -p "Enter the port to use: " port
  avrdude -p m328p -c arduino -P "$port" -b 115200 -U flash:w:"$working_dir/programme.hex"
fi

# Création du .conf
config_file="$working_dir/arduicesi"
echo "type=arduino" > "$config_file"
echo "port=$port" >> "$config_file"

# Suppression des fichiers temporaires
clean_directory() {
  local dir="$1"
  rm -f "$dir"/*
}

# Détection du mode Debug
if grep -q "DEBUG" "$working_dir/main.c"; then
  debug_mode="true"
else
  debug_mode="false"
fi

# Mode de débugage par étape
if [ "$debug_mode" = "true" ]; then
  mkdir -p "$debug_folder"
  avr-gcc -E -mmcu=atmega328p -o "$debug_folder/1_preproc.i" "$working_dir/main.c"
  avr-gcc -S -mmcu=atmega328p -o "$debug_folder/2_compil.s" "$debug_folder/1_preproc.i"
  avr-gcc -c -mmcu=atmega328p -o "$debug_folder/3_assembly.o" "$debug_folder/2_compil.s"
fi

# Affichage du message de réussite
echo ""
echo "Operations Completed Successfully"
echo ""