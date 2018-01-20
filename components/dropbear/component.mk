ifdef CONFIG_LUA_RTOS_USE_SSH_SERVER
COMPONENT_SRCDIRS := . ./libtomcrypt/src/ciphers \
./libtomcrypt/src/ciphers/aes \
./libtomcrypt/src/ciphers/safer \
./libtomcrypt/src/ciphers/twofish \
./libtomcrypt/src/encauth \
./libtomcrypt/src/encauth/ccm \
./libtomcrypt/src/encauth/eax \
./libtomcrypt/src/encauth/gcm \
./libtomcrypt/src/encauth/ocb \
./libtomcrypt/src/hashes \
./libtomcrypt/src/hashes/helper \
./libtomcrypt/src/hashes/sha2 \
./libtomcrypt/src/hashes/whirl \
./libtomcrypt/src/mac/f9 \
./libtomcrypt/src/mac/hmac \
./libtomcrypt/src/mac/pelican \
./libtomcrypt/src/math \
./libtomcrypt/src/math/fp \
./libtomcrypt/src/misc \
./libtomcrypt/src/misc/base64 \
./libtomcrypt/src/misc/crypt \
./libtomcrypt/src/misc/pkcs5 \
./libtomcrypt/src/modes \
./libtomcrypt/src/modes/cfb \
./libtomcrypt/src/modes/ctr \
./libtomcrypt/src/modes/ecb \
./libtomcrypt/src/modes/f8 \
./libtomcrypt/src/modes/lrw \
./libtomcrypt/src/modes/ofb \
./libtomcrypt/src/pk \
./libtomcrypt/src/pk/ecc \
./libtomcrypt/src/pk/asn1 \
./libtomcrypt/src/pk/asn1/der \
./libtomcrypt/src/pk/asn1/der/bit \
./libtomcrypt/src/pk/asn1/der/boolean \
./libtomcrypt/src/pk/asn1/der/choice \
./libtomcrypt/src/pk/asn1/der/ia5 \
./libtomcrypt/src/pk/asn1/der/integer \
./libtomcrypt/src/pk/asn1/der/object_identifier \
./libtomcrypt/src/pk/asn1/der/octet \
./libtomcrypt/src/pk/asn1/der/printable_string \
./libtomcrypt/src/pk/asn1/der/sequence \
./libtomcrypt/src/pk/asn1/der/set \
./libtomcrypt/src/pk/asn1/der/short_integer \
./libtomcrypt/src/pk/asn1/der/utctime \
./libtomcrypt/src/pk/asn1/der/utf8 \
./libtomcrypt/src/pk/dsa \
./libtomcrypt/src/pk/katja \
./libtomcrypt/src/pk/pkcs1 \
./libtomcrypt/src/pk/rsa \
./libtomcrypt/src/prngs \
./libtommath
COMPONENT_ADD_INCLUDEDIRS := . ./port ./libtomcrypt/src/headers ./../libssh2/include ./libtommath
else
COMPONENT_SRCDIRS :=
COMPONENT_ADD_INCLUDEDIRS :=
endif