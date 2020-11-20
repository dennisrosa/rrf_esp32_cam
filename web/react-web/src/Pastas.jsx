import React, { Component } from 'react'

export default class Pastas extends Component {

    imgEndpoint = "http://192.168.1.70/image?path=";
    endpoint = "http://192.168.1.70/lista?path=";
	
    state = {
        images: [],
        loaded: 0,
        displayText : ""
    }

    clean() {
        this.setState({ loaded: 0 })
        this.setState({ contacts: [] })
        document.getElementById('imgGif').innerHTML = "";
    };

    componentDidMount() {
        this.myInterval = setInterval(() => {

            if(this.state.loaded  <= this.state.images.length  ){
                this.setState({ loaded: this.imagesLoaded() })
            }
             
        }, 5000)
    }

    listar(path) {
		const url = this.endpoint + path
        this.setState({ images: ['a'] });

		console.log("Path" , url)
        fetch(url)
            .then(res => res.json())
            .then((data) => {

                var images  = Array.from(data.path);
                console.log(images);
                var filtered = images.filter(function(value, index, arr){ return value.path != null });
                console.log(filtered);

                this.setState({ images: filtered });
                console.log("contatcts", this.state.images);
            })
            .catch(console.log)
    };

    gerarGif() {
        console.log("Gerar");
        document.getElementById('imgGif').innerHTML = "";
        var imgs = document.querySelectorAll('img');

        var ag = new window.Animated_GIF();
        ag.setSize(640, 480);
        ag.setDelay(1);

        for (var i = 0; i < imgs.length; i++) {
            ag.addFrame(imgs[i]);
        }

        var animatedImage = document.createElement('img');
        ag.getBase64GIF(function (image) {
            animatedImage.src = image;
            var e = document.getElementById('imgGif');
            e.appendChild(animatedImage);
        });
        console.log(imgs);
        this.setState({ displayText: "" })
    }


    imagesLoaded() {
        var count = 0;
        const imgElements = document.querySelectorAll('img');
        for (let i = 0; i < imgElements.length; i += 1) {
          const img = imgElements[i];
          if (img.complete) {
            count++;
          }
        }
        return count;
      }

    onFallbackImage(ev){
        console.log("fallback image: "  +ev.target.src );

        var n = ev.target.src.search("&a");
        if (n > 0 ){
            ev.target.src = ev.target.src.substr(0,n);
        }else {
            ev.target.src = ev.target.src + "&a=" + Date.now();
        }
    }

    display(){

        var text = this.props.path ;
        if (this.state.images.length === 1){
            text +=  " - loading ....  " ;            
        } else if ( document.getElementById('imgGif')  &&  document.getElementById('imgGif').innerHTML !== ""){
            text +=  "" ;            
        } else if (this.state.loaded  !== this.state.images.length){
            text +=  " - Processing ....  " + this.state.loaded + " of " + (this.state.images.length +1) ;            
        }
        return text;
    }

    render() {

        return (
            <div>
                <div className="Pasta">
                    <h1>Pastas {this.display()} </h1>
                    { this.state.images.length > 0 & this.state.loaded === this.state.images.length  ? <button onClick={() => this.gerarGif()}>Gerar GIF</button> : null}
        
                    <div id="imgGif"></div>
                    
                    <ul>
                        {this.state.images.map((c) => (
                            <li key={c.path} >
                                {c.directory === '0' ? <img alt={c.path} onError={this.onFallbackImage}  crossOrigin="Anonymous" src={ this.imgEndpoint + c.path } /> : <div></div>}
                            </li>
                        ))}
                    </ul>
                </div>
            </div>
        )
    }
}
