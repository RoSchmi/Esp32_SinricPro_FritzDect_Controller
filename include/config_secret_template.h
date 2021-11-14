#ifndef _CONFIG_SECRET_H
#define _CONFIG_SECRET_H

// Rename config_secret_template.h into config_secret.h to activate the content


// Azure Storage Credentials
#define AZURE_CONFIG_ACCOUNT_NAME       "YourAzureStorageAccountName"
#define AZURE_CONFIG_ACCOUNT_KEY        "3M+ssz2Ws6....YourStorageAccountKey.....BOjg+1r7ZA=="

#define FRITZ_IP_ADDRESS "fritz.box"    // IP Address of FritzBox
                                        // Change for your needs
#define FRITZ_USER ""                   // FritzBox User (may be empty)
#define FRITZ_PASSWORD "MySecretName"   // FritzBox Password

#define FRITZ_DEVICE_AIN_01 ""    // AIN = Actor ID Numberof Fritz!Dect (12 digit)
#define FRITZ_DEVICE_AIN_02 ""             
#define FRITZ_DEVICE_AIN_03 ""
#define FRITZ_DEVICE_AIN_04 ""

#endif // _CONFIG_SECRET_H