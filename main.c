#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define NUMESTADOS 15
#define NUMCOLS 13
#define TAMLEX 32+1
#define TAMNOM 20+1


/******************Declaraciones Globales*************************/
FILE * in;
typedef enum{
    INICIO, FIN, LEER, ESCRIBIR, ID, CARACTER, PARENIZQUIERDO,
    PARENDERECHO, PUNTOYCOMA, COMA, ASIGNACION, SUMA, RESTA, FDT, ERRORLEXICO, MIENTRAS,
    FINMIENTRAS, SI, FINSI, REPETIRHASTA, FINREPETIRHASTA, MENOR, MAYOR, IGUAL, MENORIGUAL,
    MAYORIGUAL, IGUALDAD, DISTINTO, ENTERO, REAL,TIPO
} TOKEN;
typedef enum{
    ENT,REA,CAR
}TipoDato;
typedef struct{
    char identifi[TAMLEX];
    TOKEN t; /* t=0, 1, 2, 3 Palabra Reservada, t=ID=4 Identificador */
    TipoDato tipo;
} RegTS;

RegTS TS[1000] = { {"inicio", INICIO}, {"fin", FIN}, {"leer", LEER}, {"escribir", ESCRIBIR}, {"mientras", MIENTRAS}, {"finMientras", FINMIENTRAS}, {"si", SI},
                {"finSi", FINSI}, {"repetirHasta", REPETIRHASTA}, {"finRepetirHasta", FINREPETIRHASTA}, {"ent", TIPO,ENT}, {"real", TIPO,REA}, {"car", TIPO,CAR},{"$", 99} };

typedef struct{
    TOKEN clase;
    TipoDato tipo;
    char nombre[TAMLEX];
    int valor;
} REG_EXPRESION;

char buffer[TAMLEX];
TOKEN tokenActual;
int flagToken = 0;

/**********************Prototipos de Funciones************************/
TOKEN scanner();
int columna(int c);
int estadoFinal(int e);
void Objetivo(void);
void Programa(void);
void ListaSentencias(void);
void Sentencia(void);
void ListaIdentificadores(void);
void Identificador(REG_EXPRESION * presul,TipoDato tipo);
void ListaExpresiones(void);
void Expresion(REG_EXPRESION * presul);
void Primaria(REG_EXPRESION * presul);
void OperadorAditivo(char * presul);
void OperadorRelacional(char * presul);
REG_EXPRESION ProcesarCte(void);
REG_EXPRESION ProcesarId(TipoDato tipo);
char * ProcesarOp(void);
void Leer(REG_EXPRESION in);
void Escribir(REG_EXPRESION out);
void Pregunta(REG_EXPRESION * presul);
void Si(void);
void Mientras(void);
REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2);
void Match(TOKEN t);
TOKEN ProximoToken();
void ErrorLexico();
void ErrorSintactico();
void Generar(char * co, char * a, char * b, char * c);
char * Extraer(REG_EXPRESION * preg);
int Buscar(char * id, RegTS * TS, TOKEN * t,TipoDato* t2);
void Colocar(char * id, RegTS * TS,TipoDato tipo);
TOKEN Chequear(char * s, TipoDato tipo);
void Comenzar(void);
void Terminar(void);
void Asignar(REG_EXPRESION izq, REG_EXPRESION der);
REG_EXPRESION Caracter();
REG_EXPRESION ProcesarReal(void);
TipoDato ProcesarTipo(void);
TipoDato DecidirTipo(REG_EXPRESION e1,REG_EXPRESION e2);
void Tipo(TipoDato * presul);
void CaracterOExpresion(REG_EXPRESION * presul);
int CondicionInfijo(REG_EXPRESION e1,REG_EXPRESION e2);
void ErrorSemantico();
/***************************Programa Principal************************/
int main(int argc, char * argv[]){
    TOKEN tok;
    char nomArchi[TAMNOM];
    int l;
    /***************************Se abre el Archivo Fuente******************/
    if ( argc == 1 ){
        printf("Debe ingresar el nombre del archivo fuente (en lenguaje Micro) en la linea de comandos\n");
        return -1;
    }
    if ( argc != 2 ){
        printf("Numero incorrecto de argumentos\n");
        return -1;
    }
    strcpy(nomArchi, argv[1]);
    l = strlen(nomArchi);
    if ( l > TAMNOM ){
        printf("Nombre incorrecto del Archivo Fuente\n");
        return -1;
    }
    if ( nomArchi[l-1] != 'm' || nomArchi[l-2] != '.' ){
        printf("Nombre incorrecto del Archivo Fuente\n");
        return -1;
    }
    if ( (in = fopen(nomArchi, "r") ) == NULL ){
        printf("No se pudo abrir archivo fuente\n");
        return -1;
    }

    /*************************Inicio Compilacion***************************/
    Objetivo();
    /**************************Se cierra el Archivo Fuente******************/
    fclose(in);
    return 0;
}
    /**********Procedimientos de Analisis Sintactico (PAS) *****************/
void Objetivo(void){
    /* <objetivo> -> <programa> FDT #terminar */
    Programa();
    Match(FDT);
    Terminar();
}

void Programa(void){
/* <programa> -> #comenzar INICIO <listaSentencias> FIN */
    Comenzar();// invocacion a las rutinas semanticas, en la gramatica se coloca con #
    Match(INICIO);
    ListaSentencias();
    Match(FIN);
}
void ListaSentencias(void){
    /* <listaSentencias> -> <sentencia> {<sentencia>} */
    Sentencia();
    while ( 1 ){
        switch ( ProximoToken() ){
            case ID : case LEER : case ESCRIBIR : case SI : case MIENTRAS : case TIPO:
            Sentencia();
            break;
            default : return;
        }
    }
}

void Sentencia(void){
    TOKEN tok = ProximoToken();
    REG_EXPRESION izq, der;
    TipoDato tipo;
    switch ( tok ){ 
        case TIPO: /*<sentencia> -> <tipo> <identificador> ;*/
            Tipo(&tipo);
            Identificador(&der,tipo);
            Match(PUNTOYCOMA);
            break; 
        case ID : /* <sentencia> -> ID := <caracterOExpresion>  #asignar ; (rutina semantica)*/
            Identificador(&izq,ENT);
            Match(ASIGNACION);
            CaracterOExpresion(&der);
            Asignar(izq, der);
            Match(PUNTOYCOMA);
            break;
        case LEER : /* <sentencia> -> LEER ( <listaIdentificadores> ) */
            Match(LEER);
            Match(PARENIZQUIERDO);
            ListaIdentificadores();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;
        case ESCRIBIR : /* <sentencia> -> ESCRIBIR ( <listaExpresiones> ) */
            Match(ESCRIBIR);
            Match(PARENIZQUIERDO);
            ListaExpresiones();
            Match(PARENDERECHO);
            Match(PUNTOYCOMA);
            break;
        case SI: /* <sentencia> -> SI ( <expresion> ) <listaSentencias> FINSI */
            Si();
            break;
        case MIENTRAS: /* <sentencia> -> MIENTRAS <pregunta> ; <listaSentencias> FINMIENTRAS ; */
            Mientras();
            break;
        default : return;
    }
}

void ListaIdentificadores(void){
    /* <listaIdentificadores> -> <identificador> #leer_id {COMA <identificador> #leer_id} */
    TOKEN t;
    REG_EXPRESION reg;
    Identificador(&reg,ENT);
    Leer(reg);
    for ( t = ProximoToken(); t == COMA; t = ProximoToken() ){
        Match(COMA);
        Identificador(&reg,ENT);
        Leer(reg);
    }
}

void Identificador(REG_EXPRESION * presul,TipoDato tipo){
    /* <identificador> -> ID #procesar_id */
    Match(ID);
    *presul = ProcesarId(tipo);
}
void Tipo(TipoDato * presul){
    Match(TIPO);
    *presul = ProcesarTipo();
}

void ListaExpresiones(void){
    /* <listaExpresiones> -> <expresion> #escribir_exp {COMA <expresion> #escribir_exp} */
    TOKEN t;
    REG_EXPRESION reg;
    Expresion(&reg);
    Escribir(reg);
    for ( t = ProximoToken(); t == COMA; t = ProximoToken() ){
        Match(COMA);
        Expresion(&reg);
        Escribir(reg);
    }
}

void Expresion(REG_EXPRESION * presul){
    /* <expresion> -> <primaria> { <operadorAditivo> <primaria> #gen_infijo } */
    REG_EXPRESION operandoIzq, operandoDer;
    char op[TAMLEX];
    TOKEN t;
    Primaria(&operandoIzq);
    for ( t = ProximoToken(); t == SUMA || t == RESTA; t = ProximoToken() ){
        OperadorAditivo(op);
        Primaria(&operandoDer);
        operandoIzq = GenInfijo(operandoIzq, op, operandoDer);
    }
    *presul = operandoIzq;
}

void CaracterOExpresion(REG_EXPRESION * presul){
    /*<caracterOExpresion> -> uno de <expresion> <caracter>*/
    TOKEN tok = ProximoToken();
    if (tok==CARACTER){
        Match(CARACTER);
        *presul = Caracter();
    }
    else{
        Expresion(presul);
    }
}

REG_EXPRESION Caracter(){
    REG_EXPRESION reg;
    reg.clase=CARACTER;
    strcpy(reg.nombre,buffer);
    sscanf(buffer,"%c",&reg.valor);
    return reg;
}
void Primaria(REG_EXPRESION * presul){
    TOKEN tok = ProximoToken();
    switch ( tok ){
    case ID : /* <primaria> -> <identificador> */
        Identificador(presul,ENT);
        break;
    case ENTERO : /* <primaria> -> CONSTANTE #procesar_cte */
        Match(ENTERO);
        *presul = ProcesarCte();
        break;
    case REAL:
        Match(REAL);
        *presul = ProcesarReal();
        break;
    case PARENIZQUIERDO : /* <primaria> -> PARENIZQUIERDO <expresion> PARENDERECHO */
        Match(PARENIZQUIERDO);
        Expresion(presul);
        Match(PARENDERECHO);
        break;
    default : return;
    }
}

void OperadorAditivo(char * presul){
    /* <operadorAditivo> -> SUMA #procesar_op | RESTA #procesar_op */
    TOKEN t = ProximoToken();
    if ( t == SUMA || t == RESTA ){
        Match(t);
        strcpy(presul, ProcesarOp());
    }
    else
        ErrorSintactico(t);
}

void OperadorRelacional(char * presul){
    /* <operadorRelacional> -> MENOR | MAYOR | IGUAL | MENORIGUAL | MAYORIGUAL | DISTINTO */
    TOKEN t = ProximoToken();
    if ( t == MENOR || t == MAYOR || t == IGUAL || t == MENORIGUAL || t == MAYORIGUAL || t == DISTINTO ){
        Match(t);
        strcpy(presul, ProcesarOp());
    }
    else
        ErrorSintactico(t);
}

/**********************Rutinas Semanticas******************************/
REG_EXPRESION ProcesarCte(void){
    /* Convierte cadena que representa numero a numero entero y construye un registro semantico */
    REG_EXPRESION reg;
    reg.clase = ENTERO;
    reg.tipo = ENT;
    strcpy(reg.nombre, buffer);
    sscanf(buffer, "%d", &reg.valor);
    return reg;
}

REG_EXPRESION ProcesarReal(void){
    REG_EXPRESION reg;
    reg.clase = REAL;
    reg.tipo = REA;
    strcpy(reg.nombre, buffer);
    sscanf(buffer, "%f", &reg.valor);
    return reg;
}
REG_EXPRESION ProcesarId(TipoDato tipo){
    /* Declara ID y construye el correspondiente registro semantico */
    REG_EXPRESION reg;
    reg.clase = ID;
    reg.tipo = Chequear(buffer,tipo);
    strcpy(reg.nombre, buffer);
    return reg;
}
TipoDato ProcesarTipo(void){
    TipoDato tipo;
    TOKEN tipoEnTS;
    Buscar(buffer,TS,&tipoEnTS,&tipo);
    return tipo;

}
char * ProcesarOp(void){
    /* Declara OP y construye el correspondiente registro semantico */
    return buffer;
}

void Leer(REG_EXPRESION in){
    /* Genera la instruccion para leer */
    switch(in.tipo){
        case ENT:
            Generar("Read", in.nombre, "Entera", "");
            break;
        case CAR:
            Generar("Read", in.nombre, "Caracter", "");
            break;
        case REA:
            Generar("Read", in.nombre, "Real", "");
            break;
    }
}

void Escribir(REG_EXPRESION out){
    /* Genera la instruccion para escribir */
    switch(out.tipo){
        case ENT:
            Generar("Write", out.nombre, "Entera", "");
            break;
        case CAR:
            Generar("Write", out.nombre, "Caracter", "");
            break;
        case REA:
            Generar("Write", out.nombre, "Real", "");
            break;
    }
}

void Pregunta(REG_EXPRESION * presul){
    /* <pregunta> -> (<expresion> <comparacion> <expresion>) 
                   | (<expresion> <comparacion> <caracter>) 
                   | (<caracter> <comparacion> <expresion>) 
       
       Simplificado: (<caracterOExpresion> <comparacion> <caracterOExpresion>)
    */
    
    REG_EXPRESION op1, op2;
    char op[TAMLEX];
    
    Match(PARENIZQUIERDO);
    
    CaracterOExpresion(&op1);
    OperadorRelacional(op); 
    CaracterOExpresion(&op2);
    
    Match(PARENDERECHO);

    *presul = GenInfijo(op1, op, op2);
}

void Si(void){ 
    /* Gramática: SI <pregunta> ; <listaSentencias> FINSI ; */
    REG_EXPRESION cond;
    char etiquetaFin[20];
    static int numEtiqueta = 1;
    sprintf(etiquetaFin, "L%d", numEtiqueta++);
    Match(SI);
    
    /* Analiza la <pregunta> (que incluye los paréntesis) */
    Pregunta(&cond);
    Match(PUNTOYCOMA); 

    /* Si la condición es falsa, salta al final del bloque */
    Generar("IfFalseGoto", Extraer(&cond), "", etiquetaFin);
    
    /* Analiza las sentencias del bloque verdadero */
    ListaSentencias();
    
    Match(FINSI);
    
    Match(PUNTOYCOMA); 
    
    /* Marca el final del bloque */
    Generar("Label", etiquetaFin, "", "");

}

void Mientras(void){
    /* Gramática: MIENTRAS <pregunta> ; <listaSentencias> FINMIENTRAS ; */
    REG_EXPRESION cond;
    char etiquetaInicio[20], etiquetaFin[20];
    static int numEtiqueta = 100; // A partir de 100 para Mientras

    sprintf(etiquetaInicio, "L%d", numEtiqueta++);
    sprintf(etiquetaFin, "L%d", numEtiqueta++);

    Match(MIENTRAS);

    /* Marca el inicio del ciclo */
    Generar("Label", etiquetaInicio, "", "");

    /* Evalúa la condición */
    Pregunta(&cond);
    Match(PUNTOYCOMA);

    /* Si la condición es falsa, salta al final del bucle */
    Generar("IfFalseGoto", Extraer(&cond), "", etiquetaFin);

    /* Cuerpo del ciclo */
    ListaSentencias();

    Match(FINMIENTRAS);
    Match(PUNTOYCOMA);

     /* Salto al inicio del ciclo */
    Generar("Goto", etiquetaInicio, "", "");

    /* Marca el fin del ciclo */
    Generar("Label", etiquetaFin, "", "");
}

REG_EXPRESION GenInfijo(REG_EXPRESION e1, char * op, REG_EXPRESION e2){
    /* Genera la instruccion para una operacion infija y construye un registro semantico con el resultado*/
    if (CondicionInfijo(e1,e2)==0){
        ErrorSemantico();
    }
    REG_EXPRESION reg;
    static unsigned int numTemp = 1;
    char cadTemp[TAMLEX] ="Temp&";
    char cadNum[TAMLEX];
    char cadOp[TAMLEX];
    if ( op[0] == '-' ) strcpy(cadOp, "Restar");
    if ( op[0] == '+' ) strcpy(cadOp, "Sumar");
    if ( op[0] == '>' ) strcpy(cadOp, "CompararMayor");
    if ( strcmp(op,">=")==0 ) strcpy(cadOp, "CompararMayorIgual");
    if ( op[0] == '<' ) strcpy(cadOp, "CompararMenor");
    if ( strcmp(op,"<=")==0  ) strcpy(cadOp, "CompararMenorIgual");
    if ( strcmp(op,"==")==0  ) strcpy(cadOp, "CompararIgual");
    if ( strcmp(op,"!=")==0  ) strcpy(cadOp, "CompararDistinto");
    sprintf(cadNum, "%d", numTemp);
    numTemp++;
    strcat(cadTemp, cadNum);
    if ( e1.clase==ID) Chequear(Extraer(&e1),e1.tipo);
    if ( e2.clase == ID ) Chequear(Extraer(&e2),e2.tipo);
    Chequear(cadTemp,DecidirTipo(e1,e2));
    Generar(cadOp, Extraer(&e1), Extraer(&e2), cadTemp);
    strcpy(reg.nombre, cadTemp);
    return reg;
}
TipoDato DecidirTipo(REG_EXPRESION e1,REG_EXPRESION e2){
    if (e1.clase == ID){
        return e1.tipo;
    }
    if (e2.clase== ID){
        return e2.tipo;
    }
    if (e1.clase==ENTERO){
        return ENT;
    }
    if (e1.clase == REAL){
        return REA;
    }
}
int CondicionInfijo(REG_EXPRESION e1,REG_EXPRESION e2){
    if ((e1.clase==ID && e1.tipo==CAR) || (e2.clase==ID && e2.tipo==CAR)){
        return 0;
    }
    if(e1.clase !=e2.clase){
        if(e1.clase!=ID&&e2.clase!=ID){
            return 0;
        }
        if (e1.clase == ID){
            if (e1.tipo==ENT && e2.clase !=ENTERO){
                return 0;
            }
            if (e1.tipo==REA && e2.clase !=REAL){
                return 0;
            }
        }
        if (e2.clase == ID){
            if (e2.tipo==ENT && e1.clase !=ENTERO){
                return 0;
            }
            if (e2.tipo==REA && e1.clase !=REAL){
                return 0;
            }
        }
    }
    if (e1.clase==ID && e2.clase == ID){
        if (e1.tipo!=e2.tipo){
            return 0;
        }
    }
    return 1;
}
/***************Funciones Auxiliares**********************************/
void Match(TOKEN t){
    if ( !(t == ProximoToken()) ) ErrorSintactico();
    flagToken = 0;
}

TOKEN ProximoToken(){
    if ( !flagToken ){
        tokenActual = scanner();
        if ( tokenActual == ERRORLEXICO ) ErrorLexico();
        flagToken = 1;
        if ( tokenActual == ID){
            TipoDato t;
            Buscar(buffer, TS, &tokenActual,&t);
        }
    }
    return tokenActual;
}

void ErrorLexico(){
    printf("Error Lexico\n");
}

void ErrorSintactico(){
    printf("Error Sintactico\n");
}

void ErrorSemantico(){
    printf("Error Semantico\n");
}
void Generar(char * co, char * a, char * b, char * c){
    /* Produce la salida de la instruccion para la MV por stdout */
    printf("%s %s%c%s%c%s\n", co, a, ',', b, ',', c);
}

char * Extraer(REG_EXPRESION * preg){
    /* Retorna la cadena del registro semantico */
    return preg->nombre;
}

int Buscar(char * id, RegTS * TS, TOKEN * t,TipoDato* t2){
    /* Determina si un identificador esta en la TS */
    int i = 0;
    while ( strcmp("$", TS[i].identifi) ){
        
        if ( !strcmp(id, TS[i].identifi) ){
            *t = TS[i].t;
            *t2 = TS[i].tipo;
            return 1;
        }
        i++;
    }
    return 0;
}

void Colocar(char * id, RegTS * TS,TipoDato tipo){
    /* Agrega un identificador a la TS */
    int i = 4;
    while ( strcmp("$", TS[i].identifi) ) i++;
    if ( i < 999 ){
        strcpy(TS[i].identifi, id );
        TS[i].t = ID;
        TS[i].tipo = tipo;
        strcpy(TS[++i].identifi, "$" );
    }
}

TOKEN Chequear(char * s, TipoDato tipo){
    /* Si la cadena No esta en la Tabla de Simbolos la agrega,
    y si es el nombre de una variable genera la instruccion */
    TOKEN t;
    TipoDato tipo2;
    if ( !Buscar(s, TS, &t,&tipo2) ){
        Colocar(s, TS,tipo);
        tipo2=tipo;
        if (tipo == ENT){
            Generar("Declara", s, "Entera", "");
        }
        else if (tipo == REA){
            Generar("Declara", s, "Real", "");
        }
        else if (tipo == CAR){
            Generar("Declara", s, "Car", "");
        }
    }
    return tipo2;
}

void Comenzar(void){
    /* Inicializaciones Semanticas */
}

void Terminar(void){
    /* Genera la instruccion para terminar la ejecucion del programa */
    Generar("Detiene", "", "", "");
}

void Asignar(REG_EXPRESION izq, REG_EXPRESION der){
    /* Genera la instruccion para la asignacion */
    if ((izq.tipo==CAR&&der.clase!=CARACTER)||(izq.tipo==ENT&&der.clase!=ENTERO)||(izq.tipo==REA&&der.clase!=REAL)){
        ErrorSemantico();
    }
    Generar("Almacena", Extraer(&der), izq.nombre, "");
}

TOKEN scanner(){
    int tabla[NUMESTADOS][NUMCOLS] = { { 1, 3, 5, 6, 7, 8, 9, 10, 11, 14, 13, 0, 14 },
        { 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 },
        { 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 } };
    int car;
    int col;
    int estado = 0;
    int i = 0;
    do{
        car = fgetc(in);
        col = columna(car);
        estado = tabla[estado][col];
        if (col != 11){
            buffer[i] = car;
            i++;
        }
    }
    while (!estadoFinal(estado) && !(estado==14));

    buffer[i] = '\0';

    switch(estado){
        case 2:
            if (col != 11){
                ungetc(car,in);
                buffer[i-1] = '\0';
            }
            return ID;
        case 4:
            if (col != 11){
                ungetc(car,in);
                buffer[i-1] = '\0';
            }
            int esReal = 0;
            for(int j = 0; j < i; j++){
                if(buffer[j] == '.'){
                    esReal = 1;
                    break;
                }
            }
        return esReal ? REAL : ENTERO;
        case 5: return SUMA;
        case 6: return RESTA;
        case 7: return PARENIZQUIERDO;
        case 8: return PARENDERECHO;
        case 9: return COMA;
        case 10: return PUNTOYCOMA;
        case 12: return ASIGNACION;
        case 13: return FDT;
        case 14: return ERRORLEXICO; 
    }
    return 0;
}

int estadoFinal(int e){
    if (e==0 || e==1 || e==3 || e==11 ||e==14) return 0;
    return 1;
}

int columna(int c){
    if(isalpha(c)) return 0;
    if (isdigit(c)) return 1;
    if (c == '+') return 2;
    if (c == '-') return 3;
    if (c == '(') return 4;
    if (c == ')') return 5;
    if (c==',') return 6;
    if ( c == ';' ) return 7;
    if ( c == ':' ) return 8;
    if ( c == '=' ) return 9;
    if ( c == EOF ) return 10;
    if ( isspace(c) ) return 11;
    return 12;
}

/*************Fin Scanner**********************************************/