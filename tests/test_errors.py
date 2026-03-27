import socket
import struct

SERVER_IP = "127.0.0.1"
SERVER_PORT = 9069 

def test_rrq_nonexistent_file_returns_error_1():
    """
    This test checks if server reacts approprietly to RRQ for non-existent file.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    opcode_rrq = 1
    file_name = b"this_file_does_not_exist.txt"
    mode = b"octet"
    
    rrq_packet = struct.pack('>H', opcode_rrq) + file_name + b'\0' + mode + b'\0'

    try:
        sock.sendto(rrq_packet, (SERVER_IP, SERVER_PORT))
        response_data, server_address = sock.recvfrom(516)

        assert len(response_data) >= 4, "Invalid packet less than 4 bytes."

        received_opcode, received_error_code = struct.unpack('>HH', response_data[:4])
        assert received_opcode == 5, f"Expected OPCODE 5 (error), instead received: {received_opcode}"
        
        assert received_error_code == 1, f"Expected ERROR CODE 1, instead received: {received_error_code}"

    except socket.timeout:
        assert False, "Timeout"
    finally:
        sock.close()


def test_access_violation_returns_error_2():
    """
    This test checks if server reacts approprietly to path traversal attempt.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    opcode_rrq = 1
    file_name = b"../../etc/passwd"
    mode = b"octet"
    
    rrq_packet = struct.pack('>H', opcode_rrq) + file_name + b'\0' + mode + b'\0'

    try:
        sock.sendto(rrq_packet, (SERVER_IP, SERVER_PORT))
        response_data, server_address = sock.recvfrom(516)

        assert len(response_data) >= 4, "Invalid packet less than 4 bytes."

        received_opcode, received_error_code = struct.unpack('>HH', response_data[:4])
        assert received_opcode == 5, f"Expected OPCODE 5 (error), instead received: {received_opcode}"
        
        assert received_error_code == 2, f"Expected ERROR CODE 2, instead received: {received_error_code}"

    except socket.timeout:
        assert False, "Timeout"
    finally:
        sock.close()



def test_illegal_opcode_returns_error_4():
    """
    This test checks if server reacts approprietly to illegal TFTP operation.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    illegal_opcode = 6
    file_name = b"example_file.txt"
    mode = b"octet"
    
    rrq_packet = struct.pack('>H', illegal_opcode) + file_name + b'\0' + mode + b'\0'

    try:
        sock.sendto(rrq_packet, (SERVER_IP, SERVER_PORT))
        response_data, server_address = sock.recvfrom(516)

        assert len(response_data) >= 4, "Invalid packet less than 4 bytes."

        received_opcode, received_error_code = struct.unpack('>HH', response_data[:4])
        assert received_opcode == 5, f"Expected OPCODE 5 (error), instead received: {received_opcode}"
        
        assert received_error_code == 4, f"Expected ERROR CODE 4, instead received: {received_error_code}"

    except socket.timeout:
        assert False, "Timeout"
    finally:
        sock.close()


def test_unauthorized_client_returns_error_5():
    """
    This test check if server reacts apprioprietly to possible transfer hijacking.
    """
    sock_legit = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock_rogue = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sock_legit.settimeout(5.0)
    sock_rogue.settimeout(5.0)

    opcode_rrq = 1
    file_name = b"example_file.txt" 
    mode = b"octet"
    
    rrq_packet = struct.pack('>H', opcode_rrq) + file_name + b'\0' + mode + b'\0'
    
    rogue_ack_packet = struct.pack('>HH', 4, 1) 

    try:
        sock_legit.sendto(rrq_packet, (SERVER_IP, SERVER_PORT))
        
        response_data, server_ephemeral_addr = sock_legit.recvfrom(516)

        sock_rogue.sendto(rogue_ack_packet, server_ephemeral_addr)
        
        error_response, _ = sock_rogue.recvfrom(516)

        assert len(error_response) >= 4, "Invalid packet less than 4 bytes."
        received_opcode, received_error_code = struct.unpack('>HH', error_response[:4])
        
        assert received_opcode == 5, f"Expected OPCODE 5 (error), instead received: {received_opcode}"
        assert received_error_code == 5, f"Expected ERROR CODE 5, instead received: {received_error_code}"

    except socket.timeout:
        assert False, "Timeout"
    finally:
        sock_legit.close()
        sock_rogue.close()