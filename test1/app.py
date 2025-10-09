# based off geeksforgeeks tutorial and tokyo-s github
from flask import Flask, render_template
from flask_appconfig import AppConfig
from flask_bootstrap import Bootstrap

from fourier import fourier_blueprint
import os

# used advice from chatgpt to help setup web server
def create_app(configfile=None):

    # initialising app - create and configure flask instance
    app = Flask(__name__)
    AppConfig(app, configfile)
    Bootstrap(app)

    # set core config values here

    app.register_blueprint(fourier_blueprint) # make fourier blueprint here
    
    return app

# maybe add extra files here




