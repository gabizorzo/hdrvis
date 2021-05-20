#include <math.h>
#include <string.h>		// para usar strings

// Rotinas para acesso da OpenGL
#include "opengl.h"

#define GAMA 1.8

// Protótipos
void process();
void carregaHeader(FILE* fp);
void carregaImagem(FILE* fp, int largura, int altura);
void criaImagensTeste();

//
// Variáveis globais a serem utilizadas (NÃO ALTERAR!)
//

// Dimensões da imagem de entrada
int sizeX, sizeY;

// Header da imagem de entrada
unsigned char header[11];

// Pixels da imagem de ENTRADA (em formato RGBE)
RGBE *image;

// Pixels da imagem de SAÍDA (em formato RGB)
RGB *image8;

// Fator de exposição
float exposure;

// Histogramas
float histogram[HISTSIZE];
float adjusted[HISTSIZE];

// Flag para exibir/ocultar histogramas
unsigned char showhist = 0;

// Níveis mínimo/máximo (preto/branco)
int minLevel = 0;
int maxLevel = 255;

// Altura e largura
int minhaAltura;
int minhaLargura;




// Função principal de processamento: ela deve chamar outras funções
// quando for necessário (ex: algoritmo de tone mapping, etc)
void process()
{

    int tamanho = sizeX * sizeY;

    RGBE *ptrE = image;

    RGB *ptr = image8;

    // Aplicação do fator de Exposição

    printf("Exposure: %.3f\n", exposure);

    float expos = (float) pow(2.0,(double)exposure);


    // Leitura e Decodificação da Imagem HDF

    for(int pos=0; pos<tamanho; pos++) {
        //pega o valor da mantissa 
        unsigned char m = ptrE->e;

        //converte m para decimal 
        //int m1 = (int)m;

        //calcula o c 
        float c = (float) pow(2.0, (double)(m-136));

        //pega os valores de R G e B
        unsigned char r = ptrE -> r;
        float auxR  = ((float)r * c);

        unsigned char g = ptrE -> g;
        float auxG  = ((float)g * c);

        unsigned char b = ptrE -> b;
        float auxB  = ((float)b * c);


        auxR = (auxR * expos);
        auxG = (auxG * expos);
        auxB = (auxB * expos);

        // Tone mapping

        auxR = (auxR * 0.6);
        float auxRt = ((auxR* (2.51 * auxR + 0.03))/(auxR * (2.43 * auxR + 0.59) + 0.14));
        if (auxRt < 0.0){
            auxRt = 0.0;
        } else if (auxRt > 1.0){
            auxRt = 1.0;
        }
        
        auxG = (auxG * 0.6);
        float auxGt = ((auxG* (2.51 * auxG + 0.03))/(auxG * (2.43 * auxG + 0.59) + 0.14));
        if (auxGt < 0.0){
            auxGt = 0.0;
        } else if (auxGt > 1.0){
            auxGt = 1.0;
        } 
        
        auxB = (auxB * 0.6);
        float auxBt = ((auxB* (2.51 * auxB + 0.03))/(auxB * (2.43 * auxB + 0.59) + 0.14));
        if (auxBt < 0.0){
            auxBt = 0.0;
        } else if (auxBt > 1.0){
            auxBt = 1.0;
        }

        // Correção gama

        float auxRc = pow((double)auxRt,(1.0/GAMA));
        float auxGc = pow((double)auxGt,(1.0/GAMA));
        float auxBc = pow((double)auxBt,(1.0/GAMA));

        ptr->r = (unsigned char) (auxRc * 255.0);
        ptr->g = (unsigned char) (auxGc * 255.0);
        ptr->b = (unsigned char) (auxBc * 255.0);

        ptr++;
        ptrE++; 



        
    }

    // Histograma

    // inicializar o histogram com zeros
    for(int i=0; i<HISTSIZE; i++){
        histogram[i] = 0.0;
    }

    ptr = image8;

    for(int pos=0; pos<tamanho; pos++) {
        int l = (int)(0.299 * (float)(ptr -> r) + 0.587 * (float)(ptr -> g) + 0.114 * (float)(ptr -> b));

        histogram[l] = histogram[l]+1.0;

        ptr++;
    }

    float maior = histogram[0];
    for(int i=1; i<HISTSIZE; i++){
        if (histogram[i]>maior)
            maior = histogram[i];
    }


    for(int i=0; i<HISTSIZE; i++){
        histogram[i] = histogram[i]/maior;
    }
    


 
    // Dica: se você precisar de um vetor de floats para armazenar
    // a imagem convertida, etc, use este trecho
    // (não esqueça o free no final)
    // float *fpixels = malloc(sizeX * sizeY * 3 * sizeof(float));
    
    // NÃO ALTERAR A PARTIR DAQUI!!!!
    //
    // free(fpixels);
    buildTex();
}


int main(int argc, char** argv)
{
    if(argc==1) {
        printf("hdrvis [image file.hdf]\n");
        exit(1);
    }

    // Inicialização da janela gráfica
    init(argc,argv);

    //
    // PASSO 1: Leitura da imagem
    // A leitura do header já foi feita abaixo
    // 
    FILE* arq = fopen(argv[1], "rb");
    carregaHeader(arq);

    //1. Extrai a largura e altura do header 
    minhaLargura = header[3] + header[4]*pow(16,2) + header[5]*pow(16,1) + header[6]*pow(16,0);
    minhaAltura  = header[7] + header[8]*pow(16,2) + header[9]*pow(16,1) + header[10]*pow(16,0);

    printf("Minha largura: %d \n", minhaLargura);
    printf("Minha altura: %d \n", minhaAltura);

    carregaImagem(arq, minhaLargura, minhaAltura);
    
    // Fecha o arquivo
    fclose(arq);

    //
    // COMENTE a linha abaixo quando a leitura estiver funcionando!
    // (caso contrário, ele irá sobrepor a imagem carregada com a imagem de teste)
    //
    //criaImagensTeste();

    exposure = 0.0f; // exposição inicial

    // Aplica processamento inicial
    process();

    // Não retorna... a partir daqui, interação via teclado e mouse apenas, na janela gráfica

    // Mouse wheel é usada para aproximar/afastar
    // Setas esquerda/direita: reduzir/aumentar o fator de exposição
    // A/S: reduzir/aumentar o nível mínimo (black point)
    // K/L: reduzir/aumentar o nível máximo (white point)

    
    // ESC: finalizar o programa
   
    glutMainLoop();

    return 0;
}

// Função apenas para a criação de uma imagem em memória, com o objetivo
// de testar a funcionalidade de exibição e controle de exposição do programa
void criaImagensTeste()
{
    // TESTE: cria uma imagem de 800x600
    sizeX = 800;
    sizeY = 600;

    //printf("%d x %d\n", sizeX, sizeY);

    // Aloca imagem de entrada (32 bits RGBE)
    image = (RGBE *) malloc(sizeof(RGBE) * sizeX * sizeY);

    // Aloca memória para imagem de saída (24 bits RGB)
    image8 = (RGB *) malloc(sizeof(RGB) * sizeX * sizeY );
}

// Esta função deverá ser utilizada para ler o conteúdo do header
// para a variável header (depois você precisa extrair a largura e altura da imagem desse vetor)
void carregaHeader(FILE* fp)
{
    // Lê 11 bytes do início do arquivo
    fread(header, 11, 1, fp);
    // Exibe os 3 primeiros caracteres, para verificar se a leitura ocorreu corretamente
    //printf("Id: %c%c%c\n", header[0], header[1], header[2]);
}

// Esta função deverá ser utilizada para carregar o restante
// da imagem (após ler o header e extrair a largura e altura corretamente)
void carregaImagem(FILE* fp, int largura, int altura)
{
    sizeX = largura;
    sizeY = altura;

    // Aloca imagem de entrada (32 bits RGBE)
    image = (RGBE *) malloc(sizeof(RGBE) * sizeX * sizeY);

    // Aloca memória para imagem de saída (24 bits RGB)
    image8 = (RGB *) malloc(sizeof(RGB) * sizeX * sizeY);

    // Lê o restante da imagem de entrada
    fread(image, sizeX * sizeY * sizeof(RGBE), 1, fp);
    // Exibe primeiros 3 pixels, para verificação
    for(int i=0; i<12; i+=4) {
        //printf("%02X %02X %02X %02X\n", image[i], image[i+1], image[i+2], image[i+3]);
    }
}


