from flask import Flask, jsonify, request

app = Flask(__name__)

@app.route('/kv_storage', mertods=['GET'])
def read():

    return jsonify()


if __name__ == "__main__":
    pass
