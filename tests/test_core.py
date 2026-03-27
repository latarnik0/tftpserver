import socket
import struct

SERVER_IP = "127.0.0.1"
SERVER_PORT = 9069

def test_rrq():
    """
    This test checks if server reacts approprietly to valid RRQ.
    """
    with open("/home/latarnik3/tftpserver/data/img.jpg", "rb") as f:
        reference_data = f.read() 

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    opcode_rrq = 1
    opcode_ack = 4
    curr_block_num = 1
    file_name = b"img.jpg"
    mode = b"octet"
    
    rrq_packet = struct.pack('>H', opcode_rrq) + file_name + b'\0' + mode + b'\0'
    
    buffer = bytearray()

    try:
        sock.sendto(rrq_packet, (SERVER_IP, SERVER_PORT))
        
        while True:
            response_data, server_address = sock.recvfrom(516)

            assert len(response_data) >= 4, "Invalid packet less than 4 bytes."

            received_opcode, received_block_num = struct.unpack('>HH', response_data[:4])

            assert received_opcode == 3, f"Expected OPCODE 3 (DATA), instead received: {received_opcode}"
            assert received_block_num == curr_block_num, f"Expected Block number {curr_block_num}, instead received: {received_block_num}"

            data_chunk = response_data[4:]
            buffer.extend(data_chunk)

            ack_packet = struct.pack('>HH', opcode_ack, curr_block_num)
            sock.sendto(ack_packet, server_address)

            if len(data_chunk) < 512:
                break

            curr_block_num += 1

        assert reference_data == buffer, "Some packets were lost."

    except socket.timeout:
        assert False, "Timeout"
    finally:
        sock.close()


def test_wrq():
    """
    This test checks if server reacts approprietly to valid WRQ.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5.0)
    opcode_wrq = 2
    sent_block_num = 1
    file_name = b"wrq_test.txt"
    mode = b"octet"

    wrq_packet = struct.pack('>H', opcode_wrq) + file_name + b'\0' + mode + b'\0'

    try:
        sock.sendto(wrq_packet, (SERVER_IP, SERVER_PORT))
        response_data, server_address = sock.recvfrom(516)

        assert len(response_data) >= 4, "Invalid packet less than 4 bytes."

        received_opcode, received_block_num = struct.unpack('>HH', response_data[:4])

        assert received_opcode == 4, f"Expected Opcode 4 (ACK), instead received: {received_opcode}"
        assert received_block_num == 0, f"Expected Block number 0, instead received: {received_block_num}"

        with open("wrq_test.txt", "rb") as f:
            while True:
                chunk = f.read(512)
                data_packet = struct.pack('>HH', 3, sent_block_num) + chunk + b'\0'

                sock.sendto(data_packet, server_address)
                response_wrq_data, server_address = sock.recvfrom(516)
                
                try:
                    assert len(response_wrq_data) >= 4, "Invalid packet less than 4 bytes."

                    received_wrq_opcode, received_wrq_block_num = struct.unpack('>HH', response_wrq_data[:4])

                    assert received_wrq_opcode == 4, f"Expected Opcode 4 (ACK), instead received: {received_wrq_opcode}"
                    assert received_wrq_block_num == sent_block_num, f"Expected Block number {sent_block_num}, instead received: {received_wrq_block_num}"
                except socket.timeout:
                    assert False, "Timeout"

                if len(chunk) < 512:
                    break
    except socket.timeout:
        assert False, "Timeout"
    finally:
        sock.close()


def test_retransmission():
    """
    This test checks if server performs retransmissions of data packets corretly.
    """
    # TODO