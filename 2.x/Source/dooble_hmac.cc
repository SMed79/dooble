/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QCryptographicHash>
#include <QtDebug>

#include "dooble_hmac.h"

QByteArray dooble_hmac::sha2_512_hmac(const QByteArray &key,
				      const QByteArray &message)
{
  /*
  ** Block length is 1024 bits.
  ** Please read https://en.wikipedia.org/wiki/SHA-2.
  */

  QByteArray k(key);
  static int block_length = 1024 / CHAR_BIT;

  if(block_length < k.length())
    k = QCryptographicHash::hash(k, QCryptographicHash::Sha512);

  if(block_length > k.length())
    k.append(QByteArray(block_length - k.length(), 0));

  static QByteArray ipad(block_length, 0x36);
  static QByteArray opad(block_length, 0x5c);

  QByteArray left(block_length, 0);

  for(int i = 0; i < block_length; i++)
    left[i] = k.at(i) ^ opad.at(i);

  QByteArray right(block_length, 0);

  for(int i = 0; i < block_length; i++)
    right[i] = k.at(i) ^ ipad.at(i);

  return QCryptographicHash::hash
    (left.append(QCryptographicHash::hash(right.append(message),
					  QCryptographicHash::Sha512)),
     QCryptographicHash::Sha512);
}

QByteArray dooble_hmac::sha3_512_hmac(const QByteArray &key,
				      const QByteArray &message)
{
  /*
  ** Block length is 576 bits.
  ** Please read https://en.wikipedia.org/wiki/SHA-3.
  */

  QByteArray k(key);
  static int block_length = 576 / CHAR_BIT;

  if(block_length < k.length())
    k = QCryptographicHash::hash(k, QCryptographicHash::Sha3_512);

  if(block_length > k.length())
    k.append(QByteArray(block_length - k.length(), 0));

  static QByteArray ipad(block_length, 0x36);
  static QByteArray opad(block_length, 0x5c);

  QByteArray left(block_length, 0);

  for(int i = 0; i < block_length; i++)
    left[i] = k.at(i) ^ opad.at(i);

  QByteArray right(block_length, 0);

  for(int i = 0; i < block_length; i++)
    right[i] = k.at(i) ^ ipad.at(i);

  return QCryptographicHash::hash
    (left.append(QCryptographicHash::hash(right.append(message),
					  QCryptographicHash::Sha3_512)),
     QCryptographicHash::Sha3_512);
}

int dooble_hmac::preferred_output_size_in_bits(void)
{
  return 512;
}

int dooble_hmac::preferred_output_size_in_bytes(void)
{
  return preferred_output_size_in_bits() / CHAR_BIT;
}

/*
** http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/HMAC_SHA3-512.pdf
*/

void dooble_hmac::sha3_512_hmac_test1(void)
{
  QByteArray key
    (QByteArray::fromHex("000102030405060708090a0b0c0d0e0f"
			 "101112131415161718191a1b1c1d1e1f"
			 "202122232425262728292a2b2c2d2e2f"
			 "303132333435363738393a3b3c3d3e3f"));
  QByteArray message(QByteArray::fromHex("53616d706c65206d6573736167652066"
					 "6f72206b65796c656e3c626c6f636b6c"
					 "656e"));

  qDebug() << sha3_512_hmac(key, message).toHex();
}

void dooble_hmac::sha3_512_hmac_test2(void)
{
  QByteArray key(QByteArray::fromHex("000102030405060708090a0b0c0d0e0f"
				     "101112131415161718191a1b1c1d1e1f"
				     "202122232425262728292a2b2c2d2e2f"
				     "303132333435363738393a3b3c3d3e3f"
				     "4041424344454647"));
  QByteArray message(QByteArray::fromHex("53616d706c65206d6573736167652066"
					 "6f72206b65796c656e3d626c6f636b6c"
					 "656e"));

  qDebug() << sha3_512_hmac(key, message).toHex();
}
