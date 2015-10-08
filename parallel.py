from multiprocessing import Pool
import requests
if __name__ == '__main__':
    p = Pool(5)
    print p.map(requests.get, ['http://localhost:3150/color?bg=red'] * 5)
