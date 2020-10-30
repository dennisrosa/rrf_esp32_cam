from flask import Flask, request, Response,send_file, send_from_directory
import requests
import io
from multiprocessing import Value
from flask import json

counter = Value('i', 0)

app = Flask(__name__)
cont = 0

@app.route('/')
def hello():

    imagens = ["34004c36-b7ed-4d75-b75a-ed290b6cafef.jpg", "b2d00d26-888a-4b2e-9630-d40067072a36.jpg", "f13ce34c-9b72-4ee4-a47d-4f523bb592aa.jpg"]

    with counter.get_lock():
        cont = counter.value
        counter.value += 1
        if counter.value > 3 :
            counter.value = 0

    print(cont)
    filename = imagens[cont];
    
    cont = cont +1;    
    return send_from_directory(".", filename)


@app.route('/lista')
def listaDiretorios():
    path = request.args.get('path')
    data = ""
    if not path :
        data = "{\"path\":[{\"path\":\"\/2020-10-04T01-38\",\"directory\":\"1\"},{\"path\":\"\/2020-10-04T01-39\",\"directory\":\"1\"},{\"path\":\"\/2020-10-04T01-40\",\"directory\":\"1\"},{\"path\":\"\/2020-10-04T01-41\",\"directory\":\"1\"}]}"
    else:   
        data = "{\"path\":[{\"path\":\"\/2020-10-04T01-39\/picture1.jpg\",\"directory\":\"0\"},{\"path\":\"\/2020-10-04T01-39\/picture2.jpg\",\"directory\":\"0\"},{\"path\":\"\/2020-10-04T01-39\/picture3.jpg\",\"directory\":\"0\"},{\"path\":\"\/2020-10-04T01-39\/picture4.jpg\",\"directory\":\"0\"},{\"path\":\"\/2020-10-04T01-39\/picture5.jpg\",\"directory\":\"0\"},{\"path\":\"\/2020-10-04T01-39\/picture6.jpg\",\"directory\":\"0\"},{\"path\":\"\/2020-10-04T01-39\/picture7.jpg\",\"directory\":\"0\"}]}"

    response = app.response_class(
        response=data,
        status=200,
        mimetype='application/json'
    )
    return response


@app.route('/image/<image>')
def getImage(image):

    imagens = ["34004c36-b7ed-4d75-b75a-ed290b6cafef.jpg", "b2d00d26-888a-4b2e-9630-d40067072a36.jpg", "f13ce34c-9b72-4ee4-a47d-4f523bb592aa.jpg"]

    with counter.get_lock():
        cont = counter.value
        counter.value += 1
        if counter.value > 3 :
            counter.value = 0

    print(cont)
    filename = imagens[cont];
    
    cont = cont +1;    
    return send_from_directory(".", filename)






#from flask import Flask, render_template, request, Response
#app = Flask(__name__)

#@app.route('/')
#def hello():
#    response = requests.get("https://i.imgur.com/ExdKOOz.png")
#    image_bytes = io.BytesIO(response.content)
#    return image_bytes
       
    

#@app.route('/gencode', methods=('GET', 'POST') )    
#def gencode():  
#    return render_template('index.html')
    


