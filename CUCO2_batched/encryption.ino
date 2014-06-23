//Encrypts and decrypts using a Vignere cipher

void encrypt(char * plaintext, char * key, char *encrypted) {
  int textLength = strlen(plaintext);
  int keyLength =  strlen(key);
  for(int i=0; i < textLength; i++) {
    encrypted[i] = plaintext[i] + key[i % keyLength] - 32;
    if((unsigned) encrypted[i] >= 127) {
      encrypted[i] -= (unsigned) (127-32);
    }
  }
}

void decrypt(char *encrypted, char * key, char *plaintext) {
  int textLength = strlen(encrypted); 
  int keyLength =  strlen(key);
  for(int i=0; i < textLength; i++) {
    plaintext[i] = encrypted[i] - (key[i % keyLength] - 32);
    if(plaintext[i] < 32 || plaintext[i] >= 127) {
      plaintext[i] += (127-32);
    }
  }
}

/*
//Function that tests encryption
int main() {
  char input[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
  
  char key[2] = "";
  int len = strlen(input);
  char encrypted[len];
  char output[len];
  
  bool success = true;
  //Use ever character as the key
  for(char c = ' '; c <= '~'; c++) {
    key[0] = c;
    encrypt(input,key,encrypted);
    decrypt(encrypted,key,output);
  
    //printf("%s\n%s\n", input, output);
    //printf("%s\n", encrypted);
  
    //Check for successful decryption
    for(int i=0; i<len; i++) {
      if(input[i] != output[i]) {
        success = false;
        printf("Key:%c %d:%d ", key[0], input[i], output[i]);
      }
    }
  }
  if(success) {
    printf("Encryption worked properly");
  } 
}
*/

