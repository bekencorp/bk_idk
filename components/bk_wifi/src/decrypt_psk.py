#!/usr/bin/env python3
"""
PSK解密工具
用于解密使用XOR 0xAA加密的PSK字符串

加密算法：
1. 十六进制字符串 → 字节数组（每2个字符=1字节）
2. 每个字节与 0xAA 异或
3. 加密后的字节数组 → 十六进制字符串

解密算法（反向操作）：
1. 加密的十六进制字符串 → 字节数组
2. 每个字节与 0xAA 异或（异或是自逆的）
3. 解密后的字节数组 → 十六进制字符串
"""

PSK_ENCRYPT_KEY = 0xAA

def decrypt_psk(encrypted_str):
    """
    解密加密的PSK字符串

    参数:
        encrypted_str: 加密后的十六进制字符串

    返回:
        解密后的十六进制字符串
    """
    if not encrypted_str:
        return ""

    # Convert hex string to byte array
    # Each 2 hex characters = 1 byte
    array_len = len(encrypted_str) // 2
    if len(encrypted_str) % 2 != 0:
        array_len += 1

    data_array = []
    for i in range(0, len(encrypted_str), 2):
        hex_byte = encrypted_str[i:i+2]
        if len(hex_byte) == 2:
            data_array.append(int(hex_byte, 16))
        else:
            # Last character if odd length
            data_array.append(int(hex_byte + '0', 16) >> 4)

    # Decrypt: XOR each byte with the key
    decrypted_array = []
    for byte_val in data_array:
        decrypted_array.append(byte_val ^ PSK_ENCRYPT_KEY)

    # Convert decrypted byte array back to hex string
    decrypted = ""
    for byte_val in decrypted_array:
        decrypted += f"{byte_val:02x}"

    # Trim to original length (in case of odd length)
    if len(decrypted) > len(encrypted_str):
        decrypted = decrypted[:len(encrypted_str)]

    return decrypted


def main():
    import sys

    if len(sys.argv) > 1:
        # 从命令行参数读取加密字符串
        encrypted_str = sys.argv[1]
    else:
        # 默认使用用户提供的加密字符串（十六进制格式）
        encrypted_str = '185d0de99d45ed33d12b19afdef15c84'

    print("=" * 60)
    print("PSK解密工具")
    print("=" * 60)
    print(f"加密字符串: {encrypted_str}")
    print(f"密钥: 0x{PSK_ENCRYPT_KEY:02X}")
    print("-" * 60)

    decrypted = decrypt_psk(encrypted_str)

    print(f"解密结果: {decrypted}")
    print("=" * 60)

    return decrypted


if __name__ == "__main__":
    main()
