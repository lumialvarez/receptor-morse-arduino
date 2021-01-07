/*
    Receptor_Morse.ino
    Autor: Luis Miguel Alvarez
    Fecha: 25/10/2014
    Receptor de codigo Morse por medio de un sistema optico (fotoresistor), 
    lee el mensaje pulsos luminicos codificados en clave Morse y 
    posteriormente es enviado por medio del serial. El mensaje es recibido 
    en minusculas.
    
    - Envie "ON" para encender la informacion detallada de la recepcion de 
        los pulsos de luminicos.
    - Envie "OFF para apagar la informacion detallada(por defecto).
    
    Probado con Arduino IDE 1.0.5, tarjetas Arduino MEGA y Leonardo.
*/


// Codigos Morce del Alfabeto
char* letras[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", // A-I
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", // J-R
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.." // S-Z
};

// Codigos Morce de los Numeros
char* numeros[] = {
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
  "---..", "----."}; // 0-9


int PinEntrada = A0; // Pin de la entrada de datos
int PinLED = 13; // // Pin salida LED confirmacion

int t_MUESTRA = 10; //Tiempo en milisegundos en que se toma las muestras
int t_PUNTO = 200;
int t_RAYA = 600;
int t_FIN_LETRA = 800;
int t_FIN_PALABRA = 1800;
int t_ERROR = 160;
boolean DEBUG = false;
boolean ALTO = false;
boolean BAJO = false;
unsigned int tiempo = 0;
String buffer = "";
String mensaje = "";

short LF = 10;        // ASCII linefeed (Nueva linea)

void setup(){
  Serial.begin(115200);
  pinMode(PinLED,OUTPUT);
  
  Serial.println("---------------------------------------");
  Serial.println("Bienvenido al Receptor de clave morse");
  Serial.println("---------------------------------------");
  Serial.println("Es espera de mensajes...");
}


/* 
   Este metodo principal que se repite "infinitamente", se maneja mediante la 
   senal digital entrante (0 - 1) correspondientes a los estados "BAJO" y "ALTO".
   La funcion de este metodo establecer el tiempo aproximado en que la senal 
   permanece en "BAJO" o "ALTO".
   Tambien permite el manejo del estado "DEBUG" para mostrar detalles del 
   funcionamiento.
   * Si la entrada por la consola SERIAL es igual a "on" permite que se muestre 
   detalladamente las entradas y el funcionamiento en general del receptor, lo 
   cual permitira establecer problemas.
   * Si la entrada por la consola SERIAL es igual a "off" esconde las entradas 
   (POR DEFECTO).
   
   * Mediante condicionales excluyentes, se cuenta el tiempo que dura el estado
   "ALTO" o "BAJO". Con la unica condicion de que en el estado "BAJO" tiene un 
   tiempo maximo (llamado del metodo tiempo_excedido), con lo cual se da por 
   terminado la recepcion del mensaje (cuando lo hay).
     
    Nota: El estado de la entrada de datos se representa en el led integrado a 
    la placa Arduino PinLED (pin numero 13). 
*/
void loop(){
  if(Serial.available()>0){
    String comando = Serial.readStringUntil(LF);
    if(comando.equals("on")){
      DEBUG = true;
      Serial.println("Debug ON");
    }
    else if(comando.equals("off")){
      DEBUG = false;
      Serial.println("Debug OFF");
    }
  }

  boolean ESTADO = digitalRead(PinEntrada);
  digitalWrite(PinLED, ESTADO);


  if(ESTADO && !ALTO){
    ALTO = true;
  }
  else if( ESTADO && ALTO){
    tiempo++;
  }
  else if(!ESTADO && ALTO){
    ALTO = false;
    decodificar_encendido(tiempo * t_MUESTRA);
    tiempo = 0;
  }


  if(!ESTADO && !BAJO){
    BAJO = true;
  }
  else if( !ESTADO && BAJO){
    tiempo++;
    if((tiempo * t_MUESTRA) > (t_FIN_PALABRA + t_ERROR)){
      BAJO = false;
      tiempo_excedido();
    }
  }
  else if(ESTADO && BAJO){
    BAJO = false;
    decodificar_apagado(tiempo * t_MUESTRA);
    tiempo = 0;
  }
  delay(t_MUESTRA);
}


/* 
   Este metodo determinada si la senal entrante (en "ALTO") es una "raya" o un 
   "punto" por medio de la duracion y la guarda en la variable global "buffer".
  
   @param t_on Entero con que representa una aproximacion en milisegundos la
               duracion en que la senal estuvo en "ALTO".
*/
void decodificar_encendido(int t_on){
  if(DEBUG){
    Serial.print(t_on);
    Serial.print(" Milis ON");
  }
  if(t_on > (t_PUNTO - t_ERROR) && t_on < (t_PUNTO + t_ERROR)){
    if(DEBUG){
      Serial.println("  .");
    }
    buffer.concat(".");
  }
  else if(t_on > (t_RAYA - t_ERROR) && t_on < (t_RAYA + t_ERROR)){
    if(DEBUG){
      Serial.println("  -");
    }
    buffer.concat("-");
  }
  else{
    if(DEBUG){
      Serial.println();
    }
  }
}


/* 
   Este metodo determinada si la senal entrante (en "BAJO") es espacio entre 
   caracteres o un espacio entre palabras; por medio de la duracion de dicha 
   senal.
   * Si es un espacio entre caracteres, busca su significado y lo concatena a 
   la variable global "mensaje" y borra "buffer".
   * Si es un un espacio entre palabras, concatena un espacio en blanco a la 
   variable global "mensaje" y borra "buffer".
     
   @param t_off Entero con que representa una aproximacion en milisegundos l
                duracion en que la senal estuvo en "BAJO".
*/
void decodificar_apagado(int t_off){
  if(DEBUG){
    Serial.print(t_off);
    Serial.print(" Milis OFF");
  }
  if(t_off > (t_FIN_LETRA - t_ERROR) && t_off < (t_FIN_LETRA + t_ERROR)){
    if(DEBUG){
      Serial.println("  fin letra ");
    }
    char letra = encontrar_letra(buffer);
    Serial.println(buffer + " -> " + letra);
    buffer = "";
  }
  else if(t_off > (t_FIN_PALABRA - t_ERROR) && t_off < (t_FIN_PALABRA + t_ERROR)){
    char letra = encontrar_letra(buffer);
    Serial.println(buffer + " -> " + letra);
    buffer = "";
    mensaje.concat(" ");
    Serial.println();
    if(DEBUG){
      Serial.println("  fin palabra");
    }
  }
  else {
    if(DEBUG){
      Serial.println();
    }
  }
}


/* 
   Este metodo es llamado cuando la senal en estado "BAJO" sobrepasa de 
   determinado tiempo. Ademas llama el metodo de limpiar variables.
   * Si hay algun mensaje guardado en la variable global "mensaje" es mostrado, 
  con lo que se da por terminada la recepcion del mensaje.
*/
void tiempo_excedido(){
  BAJO = false;
  if(mensaje != ""){
    if(DEBUG){
      Serial.println("  fin palabra");
    }
    char letra = encontrar_letra(buffer);
    Serial.println(buffer + " -> " + letra);
    buffer = "";
    Serial.println("Recepcion terminada....");
    Serial.println("Mensaje recibido: \"" + mensaje + "\"");
  }
  limpiar_variables();
}


/* 
   Este metodo determinada que letra corresponde a una serie de rayas y puntos.
   Primero busca en el alfabeto, si no es encontrada se busca en los numeros.
   * Si la secuencia es encontrada (alfabeto o numeros) es guardado el caracter 
   correspondiente en la variable global "mensaje"
     
   @param codigo Cadena que representa una secuencia de rayas y punto para ser 
                 buscado su caracter correspondiente.
                 
   @return Devuelve el caracter correspondiente a la secuencia de rayas y puntos 
           de la entrada.
*/
char encontrar_letra(String codigo){
  char letra;
  boolean encontrado = false;
  //Busqueda de la secuencia en el alfabeto
  for(int i = 0 ; i < sizeof(letras) ; i++){
    if(codigo.equals(String(letras[i])) && !encontrado){
      letra = char(int('a') + i);
      if(letra >= 'a' && letra <= 'z'){ // verificacion final
        encontrado = true;
      }
    }
  }
  //Busqueda de la secuencia en los numero si no fue encontrado en el alfabeto
  if(!encontrado){
    for(int i = 0 ; i < sizeof(numeros) ; i++){
      if(codigo.equals(String(numeros[i]))){
        letra = char(int('0') + i);
        if(letra >= '0' && letra <= '9'){ // verificacion final
          encontrado = true;
        }
      }
    }
  }
  
  if(encontrado){
    mensaje.concat(letra);
  }
  return letra;
}


/* 
  Este metodo limpia la variables globales
*/
void limpiar_variables(){
  tiempo = 0;
  buffer = "";
  mensaje = "";
}

