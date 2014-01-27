/**
 * Created with IntelliJ IDEA.
 * User: nvt
 * Date: 10/15/13
 * Time: 4:37 PM
 * To change this template use File | Settings | File Templates.
 */

import java.nio.{ByteOrder, ByteBuffer}
import scala.collection.mutable.ArrayBuffer
import scala.collection.mutable


object HuffmanEncoder {
	class BitBuffer {
		var intern = ArrayBuffer[Byte]()
		var curBit = 0
		var curByte = 0
		var totalBits = 0

		def appendBit ( i : Int ) {
			if ( i != 0 ) {
				curByte |= (1 << curBit)
			}
			curBit += 1
			if ( curBit == 8 ) {
				intern.append(curByte.toByte)
				curByte = 0
				curBit = 0
			}
			totalBits += 1
		}
		def flush () {
			if ( curBit != 0 ) {
				intern.append(curByte.toByte)
				curBit = 0
				curByte = 0
			}
		}

		def foreach[T] ( f : (Int) => T ) {
			var bitCount = 0
			var bitIter = 0
			var byteIter = 0
			while ( byteIter < intern.size && bitCount < totalBits ) {
				val r = if ( (intern(byteIter) & (1 << bitIter)) != 0 ) { 1 } else { 0 }
				f( r )
				bitIter += 1
				if ( bitIter == 8 ) {
					bitIter = 0
					byteIter += 1
				}
				bitCount += 1
			}
		}
	}

	protected case class HuffmanNode ( byte : Byte , probability : Float ) {
		var isEndCode = false
		def isLeaf: Boolean = left.isEmpty

		var left : Option[HuffmanNode] = None
		var right : Option[HuffmanNode] = None
	}


	case class HuffmanResult ( bookPackets : List[BookPacket], dataPackets : List[DataPacket]) {

	}

	case class BookPacket (unused : Byte, data : Array[Byte]) {
		def raw = {
			val bb = ByteBuffer.allocate(20)
			bb.put(unused)
			bb.put(data)
			bb.array()
		}
	}
	case class DataPacket (offset : Int, var numMessages : Byte, data : Array[Byte]){
		def raw = {
			val bb = ByteBuffer.allocate(20)
			bb.order(ByteOrder.nativeOrder())
			bb.putShort(offset.toShort)
			bb.put(numMessages)
			if ( data.size != 17 ) { println("WAT : " + data.size) }
			bb.put(data)
			bb.array()
		}
	}
	
	def bookPacketsFromRaw ( book : ArrayBuffer[Byte] ) = {
		for ( i <- 0 until book.size by 19 ) yield {
			val start = i
			val end = i + math.min(book.size - start,19)

			val data = Array.ofDim[Byte](19)
			for ( b <- start until end ) {
				data(b - start) = book(b)
			}

			BookPacket(-1.toByte,data)
		}
	}

	def dataPacketsFromCodes(codeBuffer: ArrayBuffer[List[Int]],endCode : List[Int]) : Array[DataPacket] = {
		val dataPackets = ArrayBuffer[DataPacket]()

		val bitsPerPacket = 17 * 8

		var startIndex = 0
		var endIndex = 0
		var totalBits = 0
		while ( startIndex < codeBuffer.size ) {
			endIndex = startIndex
			while ( endIndex < codeBuffer.size && totalBits + codeBuffer(endIndex).size <= bitsPerPacket ) {
				totalBits += codeBuffer(endIndex).size
				endIndex += 1
			}

			val bitBuffer = new BitBuffer
			for ( i <- startIndex until endIndex ; bit <- codeBuffer(i) ) {
				bitBuffer.appendBit(bit)
			}
			for ( bit <- endCode ; if totalBits < bitsPerPacket ) {
				totalBits += 1
				bitBuffer.appendBit(bit)
			}

			totalBits = 0

			bitBuffer.flush()
			val data = bitBuffer.intern
			dataPackets.append(DataPacket(startIndex,0,data.toArray))

			startIndex = endIndex
		}

		assert( dataPackets.size < 256 )
		dataPackets.foreach( _.numMessages = dataPackets.size.toByte )

		dataPackets.toArray
	}

	def encodeBytes ( binaryBuffer : ArrayBuffer[Byte] ) = {
		val byteCounts = new mutable.HashMap[Byte,Int]

		for ( byte <- binaryBuffer ) {
			byteCounts(byte) = byteCounts.getOrElse(byte,0) + 1
		}
		val total = binaryBuffer.size

		implicit val nodeOrdering = new Ordering[HuffmanNode] {
			def compare(x: HuffmanNode, y: HuffmanNode): Int = y.probability compare x.probability
//				if ( x.probability < y.probability ) { 1 }
//				else if ( x.probability > y.probability ) { -1 }
//				else { 0 }
		}
		val Q = new mutable.PriorityQueue[HuffmanNode]


		for ( (byte,count) <- byteCounts ) {
			val hb = HuffmanNode(byte,count.toFloat / math.max(total.toFloat,1.0f))
			Q.enqueue(hb)
		}
		val endNode = HuffmanNode(0,0.0f)
		endNode.isEndCode = true
		Q.enqueue(endNode)

		var root : HuffmanNode = null
		while ( Q.nonEmpty ) {
			val A = Q.dequeue()
			if ( Q.nonEmpty ) {
				val B = Q.dequeue()

				val nn = HuffmanNode(0,A.probability + B.probability)
				nn.left = Some(A)
				nn.right = Some(B)
				Q.enqueue( nn )
			} else {
				root = A
			}
		}

		var endCode : List[Int] = Nil
		val table = Array.ofDim[List[Int]](256)
		def createTable ( node : HuffmanNode , bits : List[Int] ) {
			if ( node.isEndCode ) { endCode = bits }
			else {
				if ( node.isLeaf ) {
					table(node.byte & 0xff) = bits
				} else {
					createTable( node.left.get , bits :+ 0 )
					createTable( node.right.get , bits :+ 1 )
				}
			}
		}
		createTable(root,Nil)

		val codeBuffer = ArrayBuffer[List[Int]]()
		for ( rawByte <- binaryBuffer ; byte = rawByte & 0xff ) {
			if ( table(byte) == null ) { throw new IllegalStateException("byte with no code : " + (byte & 0xff)) }
			codeBuffer.append(table(byte))
		}


		val dataPackets = dataPacketsFromCodes( codeBuffer , endCode )

		val book = ArrayBuffer[Byte]()
		def writeBook ( node : HuffmanNode ) {
			if ( node.isLeaf ) {
				book.append(node.byte)
			} else {
				var internalNodeByte = 0
				if ( node.right.get.isLeaf ) { internalNodeByte |= 0x01 }
				if ( node.left.get.isLeaf ) { internalNodeByte |= 0x02 }

				val startIndex = book.size
				book.append(internalNodeByte.toByte)
				if ( book(startIndex) != internalNodeByte.toByte ) { println("WAT") }

				writeBook(node.right.get)

				val jumpIndex = book.size
				val jumpDelta = if ( node.left.get.isEndCode ) { 128 } else { jumpIndex - startIndex }
				println(jumpDelta)
				if ( jumpDelta > (1 << 7) || jumpDelta % 2 != 0 ) {
					throw new IllegalStateException(s"Invalid jumpDelta, either odd (how?) or out of range [2,128), value was $jumpDelta")
				}

				internalNodeByte |= (((jumpDelta-2) >> 1) << 2)
				if ( internalNodeByte >= (Byte.MaxValue+1) * 2) {
					throw new IllegalStateException(s"Internal node byte is outside the valid range, is $internalNodeByte")
				}
				book(startIndex) = internalNodeByte.toByte

				writeBook(node.left.get)
			}
		}
		writeBook(root)

		val bookPackets = bookPacketsFromRaw(book)

		HuffmanResult( bookPackets.toList , dataPackets.toList)
	}
}

