import random

CACHE_SIZE = 1024
LINES = 10000


def get_words(filename):
    f = open(filename, "r")
    words = f.read()
    word_list = words.split("\n")
    return word_list


def get_kv_string(kv_command, key, value):
    return kv_command + " " + key + " " + str(value) + "\n"


def main():
    word_file = "/usr/share/dict/words"
    word_list = get_words(word_file)
    # Limit the word list to guarantee repetitions
    word_list = word_list[:1000]
    command_file = open("simulation.txt", "w")
    count = 0
    kv_commands = ["PUT", "GET", "DEL"]

    random.seed(None)

    while count < CACHE_SIZE:
        word_index = random.randint(0, len(word_list) - 1)
        value = random.randint(0, 150000)
        kv_string = get_kv_string(kv_commands[0], word_list[word_index], value)

        command_file.write(kv_string)

        count += 1

    count = 0

    while count < LINES:
        word_index = random.randint(0, len(word_list) - 1)
        comm_index = random.randint(0, len(kv_commands) - 1)
        value = random.randint(0, 150000)
        kv_string = get_kv_string(
                kv_commands[comm_index], word_list[word_index], value)

        command_file.write(kv_string)

        count += 1

    command_file.close()


if __name__ == "__main__":
    main()
